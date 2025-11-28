#include "../include/ioc_gaialib/concurrent_multifile_catalog_v2.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <thread>

namespace ioc {
namespace gaia {

ConcurrentMultiFileCatalogV2::ConcurrentMultiFileCatalogV2(const std::string& catalog_dir, 
                                                           size_t max_cached_chunks)
    : catalog_dir_(catalog_dir), max_cached_chunks_(max_cached_chunks) {
    
    if (!loadMetadata()) {
        throw std::runtime_error("Failed to load catalog metadata from: " + catalog_dir);
    }
    
    // Reserve space to avoid reallocations during concurrent access
    chunk_cache_.reserve(max_cached_chunks_ * 2);
}

ConcurrentMultiFileCatalogV2::~ConcurrentMultiFileCatalogV2() = default;

bool ConcurrentMultiFileCatalogV2::loadMetadata() {
    std::string metadata_path = catalog_dir_ + "/metadata.dat";
    std::ifstream file(metadata_path, std::ios::binary);
    
    if (!file) {
        std::cerr << "Cannot open metadata file: " << metadata_path << std::endl;
        return false;
    }
    
    // Read header
    file.read(reinterpret_cast<char*>(&header_), sizeof(header_));
    if (!file) {
        std::cerr << "Failed to read catalog header" << std::endl;
        return false;
    }
    
    // Read HEALPix index
    healpix_index_.resize(header_.num_healpix_pixels);
    file.read(reinterpret_cast<char*>(healpix_index_.data()), 
              header_.num_healpix_pixels * sizeof(HEALPixIndexEntry));
    
    if (!file) {
        std::cerr << "Failed to read HEALPix index" << std::endl;
        return false;
    }
    
    // Read chunk index  
    chunk_index_.resize(header_.total_chunks);
    file.read(reinterpret_cast<char*>(chunk_index_.data()), 
              header_.total_chunks * sizeof(ChunkInfo));
    
    return file.good();
}

std::shared_ptr<ConcurrentMultiFileCatalogV2::ChunkData> 
ConcurrentMultiFileCatalogV2::getOrLoadChunk(uint64_t chunk_id) {
    
    // First check with read lock (fast path for cache hits)
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex_);
        auto it = chunk_cache_.find(chunk_id);
        if (it != chunk_cache_.end()) {
            it->second->last_access = std::chrono::steady_clock::now();
            cache_hits_++;
            return it->second;
        }
    }
    
    // Cache miss - need to load chunk with write lock
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    // Double-check after acquiring write lock
    auto it = chunk_cache_.find(chunk_id);
    if (it != chunk_cache_.end()) {
        it->second->last_access = std::chrono::steady_clock::now();
        cache_hits_++;
        return it->second;
    }
    
    // Actually load the chunk
    cache_misses_++;
    auto chunk_data = loadChunk(chunk_id);
    
    if (!chunk_data) {
        return nullptr;
    }
    
    // Evict old chunks if cache is full
    if (chunk_cache_.size() >= max_cached_chunks_) {
        evictLRUChunks();
    }
    
    chunk_cache_[chunk_id] = chunk_data;
    return chunk_data;
}

std::shared_ptr<ConcurrentMultiFileCatalogV2::ChunkData> 
ConcurrentMultiFileCatalogV2::loadChunk(uint64_t chunk_id) {
    
    if (chunk_id >= header_.total_chunks) {
        return nullptr;
    }
    
    std::string chunk_path = getChunkPath(chunk_id);
    std::ifstream file(chunk_path, std::ios::binary);
    
    if (!file) {
        std::cerr << "Cannot open chunk file: " << chunk_path << std::endl;
        return nullptr;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    size_t num_records = file_size / sizeof(Mag18RecordV2);
    std::vector<Mag18RecordV2> records(num_records);
    
    file.read(reinterpret_cast<char*>(records.data()), file_size);
    
    if (!file) {
        std::cerr << "Failed to read chunk data: " << chunk_path << std::endl;
        return nullptr;
    }
    
    return std::make_shared<ChunkData>(chunk_id, std::move(records));
}

std::vector<GaiaStar> ConcurrentMultiFileCatalogV2::queryCone(double ra, double dec, double radius, 
                                                              size_t max_results) {
    active_readers_++;
    std::vector<GaiaStar> results;
    
    // Calculate Dec bounds for quick filtering
    const double dec_min = std::max(-90.0, dec - radius);
    const double dec_max = std::min(90.0, dec + radius);
    
    // Calculate RA bounds (accounting for cos(dec) factor)
    const double cos_dec = std::cos(dec * M_PI / 180.0);
    const double ra_margin = (cos_dec > 0.01) ? radius / cos_dec : 180.0;
    double ra_min = ra - ra_margin;
    double ra_max = ra + ra_margin;
    
    // Handle RA wrap-around
    bool crosses_zero = (ra_min < 0 || ra_max > 360);
    if (ra_min < 0) ra_min += 360;
    if (ra_max > 360) ra_max -= 360;
    
    // Scan all chunks (they're not sorted by position, so we must check all)
    // This is actually fast because we only load chunks that might have matching stars
    for (uint64_t chunk_id = 0; chunk_id < header_.total_chunks; ++chunk_id) {
        
        // Get chunk data (thread-safe with caching)
        auto chunk_data = getOrLoadChunk(chunk_id);
        if (!chunk_data) continue;
        
        // Read lock for accessing chunk records (allows multiple readers)
        std::shared_lock<std::shared_mutex> chunk_lock(chunk_data->access_mutex);
        const auto& chunk_records = chunk_data->records;
        
        // Process stars in this chunk
        for (const auto& record : chunk_records) {
            // Quick bounding box check first (very fast)
            if (record.dec < dec_min || record.dec > dec_max) continue;
            
            // RA check with wrap-around handling
            bool ra_ok;
            if (crosses_zero) {
                ra_ok = (record.ra >= ra_min || record.ra <= ra_max);
            } else {
                ra_ok = (record.ra >= ra_min && record.ra <= ra_max);
            }
            if (!ra_ok) continue;
            
            // Precise angular distance check
            double dist = angularDistance(ra, dec, record.ra, record.dec);
            if (dist <= radius) {
                results.push_back(recordToStar(record));
                
                if (max_results > 0 && results.size() >= max_results) {
                    active_readers_--;
                    return results;
                }
            }
        }
    }
    
    active_readers_--;
    return results;
}

std::optional<GaiaStar> ConcurrentMultiFileCatalogV2::queryBySourceId(uint64_t source_id) {
    active_readers_++;
    
    // Search all chunks (source_id is not indexed, so linear scan required)
    for (uint64_t chunk_id = 0; chunk_id < header_.total_chunks; ++chunk_id) {
        auto chunk_data = getOrLoadChunk(chunk_id);
        if (!chunk_data) continue;
        
        std::shared_lock<std::shared_mutex> chunk_lock(chunk_data->access_mutex);
        const auto& chunk_records = chunk_data->records;
        
        // Binary search if records are sorted by source_id within chunk
        // Otherwise linear search
        for (const auto& record : chunk_records) {
            if (record.source_id == source_id) {
                active_readers_--;
                return recordToStar(record);
            }
        }
    }
    
    active_readers_--;
    return std::nullopt;
}

void ConcurrentMultiFileCatalogV2::evictLRUChunks() {
    // Remove 25% of cached chunks (LRU)
    size_t to_remove = max_cached_chunks_ / 4;
    if (to_remove == 0) to_remove = 1;
    
    std::vector<std::pair<std::chrono::steady_clock::time_point, uint64_t>> access_times;
    access_times.reserve(chunk_cache_.size());
    
    for (const auto& [chunk_id, chunk_data] : chunk_cache_) {
        access_times.emplace_back(chunk_data->last_access, chunk_id);
    }
    
    // Sort by access time (oldest first)
    std::sort(access_times.begin(), access_times.end());
    
    // Remove oldest chunks
    for (size_t i = 0; i < std::min(to_remove, access_times.size()); i++) {
        chunk_cache_.erase(access_times[i].second);
    }
}

// Copy implementations from original multifile catalog for HEALPix, etc.
uint32_t ConcurrentMultiFileCatalogV2::getHEALPixPixel(double ra, double dec) const {
    double theta = (90.0 - dec) * M_PI / 180.0;  // Colatitude
    double phi = ra * M_PI / 180.0;               // Longitude
    return ang2pix_nest(theta, phi);
}

uint32_t ConcurrentMultiFileCatalogV2::ang2pix_nest(double theta, double phi) const {
    const uint32_t nside = header_.healpix_nside;
    const double z = cos(theta);
    const double za = fabs(z);
    const double TWOPI = 2.0 * M_PI;
    const double HALFPI = M_PI / 2.0;
    
    if (za <= 2.0/3.0) {
        const double temp1 = nside * (0.5 + phi / TWOPI);
        const double temp2 = nside * z * 0.75;
        const uint32_t jp = static_cast<uint32_t>(temp1 - temp2);
        const uint32_t jm = static_cast<uint32_t>(temp1 + temp2);
        const uint32_t ir = nside + 1 + jp - jm;
        const uint32_t kshift = 1 - (ir & 1);
        const uint32_t ip = (jp + jm - nside + kshift + 1) / 2;
        
        const uint32_t face_num = ((jp < nside) ? 0 : 
                                   (jm < nside) ? 2 : 
                                   (jp >= 2*nside) ? 4 : 
                                   (jm >= 2*nside) ? 6 : 8) + (ir - nside);
        
        return face_num * nside * nside + (ip * nside + (ir - nside + 1));
    }
    
    // Polar caps
    const uint32_t ntt = static_cast<uint32_t>(phi * 4.0 / TWOPI);
    const double tp = phi - ntt * TWOPI / 4.0;
    const double tmp = sqrt(3.0 * (1.0 - za));
    const uint32_t jp = static_cast<uint32_t>(nside * tp / HALFPI * tmp);
    const uint32_t jm = static_cast<uint32_t>(nside * (1.0 - tp / HALFPI) * tmp);
    const uint32_t ir = jp + jm + 1;
    const uint32_t ip = ntt + 1;
    
    if (z > 0) {
        return 2 * ir * (ir - 1) + ip - 1;
    } else {
        const uint32_t npix = 12 * nside * nside;
        return npix - 2 * ir * (ir + 1) + ip - 1;
    }
}

std::vector<uint32_t> ConcurrentMultiFileCatalogV2::getPixelsInCone(double ra, double dec, double radius) const {
    std::vector<uint32_t> pixels;
    
    const double deg_per_pixel = 180.0 / (4.0 * header_.healpix_nside);  
    const double dec_min = std::max(-90.0, dec - radius);
    const double dec_max = std::min(90.0, dec + radius);
    const double ra_width = radius / std::cos(dec * M_PI / 180.0);
    
    for (double test_dec = dec_min; test_dec <= dec_max; test_dec += deg_per_pixel) {
        for (double test_ra = ra - ra_width; test_ra <= ra + ra_width; test_ra += deg_per_pixel) {
            double norm_ra = test_ra;
            while (norm_ra < 0) norm_ra += 360;
            while (norm_ra >= 360) norm_ra -= 360;
            
            if (angularDistance(ra, dec, norm_ra, test_dec) <= radius) {
                uint32_t pixel = getHEALPixPixel(norm_ra, test_dec);
                pixels.push_back(pixel);
            }
        }
    }
    
    std::sort(pixels.begin(), pixels.end());
    pixels.erase(std::unique(pixels.begin(), pixels.end()), pixels.end());
    
    return pixels;
}

double ConcurrentMultiFileCatalogV2::angularDistance(double ra1, double dec1, double ra2, double dec2) const {
    const double deg2rad = M_PI / 180.0;
    const double dra = (ra2 - ra1) * deg2rad;
    const double ddec = (dec2 - dec1) * deg2rad;
    const double a = std::sin(ddec/2) * std::sin(ddec/2) + 
                    std::cos(dec1 * deg2rad) * std::cos(dec2 * deg2rad) * 
                    std::sin(dra/2) * std::sin(dra/2);
    return 2.0 * std::atan2(std::sqrt(a), std::sqrt(1-a)) / deg2rad;
}

GaiaStar ConcurrentMultiFileCatalogV2::recordToStar(const Mag18RecordV2& record) const {
    GaiaStar star;
    star.source_id = record.source_id;
    star.ra = record.ra;
    star.dec = record.dec;
    star.parallax = record.parallax;
    star.pmra = record.pmra;
    star.pmdec = record.pmdec;
    star.phot_g_mean_mag = record.g_mag;
    star.bp_rp = record.bp_rp;
    star.phot_bp_mean_mag = record.bp_mag;
    star.phot_rp_mean_mag = record.rp_mag;
    return star;
}

ConcurrentMultiFileCatalogV2::ConcurrencyStats ConcurrentMultiFileCatalogV2::getStats() const {
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    
    size_t memory_used = 0;
    for (const auto& [chunk_id, chunk_data] : chunk_cache_) {
        memory_used += chunk_data->records.size() * sizeof(Mag18RecordV2);
    }
    
    size_t total_queries = cache_hits_.load() + cache_misses_.load();
    double hit_rate = total_queries > 0 ? (cache_hits_.load() * 100.0) / total_queries : 0.0;
    
    return {
        cache_hits_.load(),
        cache_misses_.load(),
        chunk_cache_.size(),
        memory_used / (1024 * 1024),  // Convert to MB
        active_readers_.load(),
        hit_rate
    };
}

void ConcurrentMultiFileCatalogV2::clearCache() {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    chunk_cache_.clear();
}

void ConcurrentMultiFileCatalogV2::preloadChunks(const std::vector<uint64_t>& chunk_ids) {
    for (uint64_t chunk_id : chunk_ids) {
        getOrLoadChunk(chunk_id);
    }
}

std::string ConcurrentMultiFileCatalogV2::getChunkPath(uint64_t chunk_id) const {
    return catalog_dir_ + "/chunks/chunk_" + 
           std::string(3 - std::to_string(chunk_id).length(), '0') + 
           std::to_string(chunk_id) + ".dat";
}

} // namespace gaia
} // namespace ioc