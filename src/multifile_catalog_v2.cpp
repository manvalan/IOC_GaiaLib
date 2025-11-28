/**
 * Multi-File V2 Catalog Reader Implementation
 */

#include "ioc_gaialib/multifile_catalog_v2.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace fs = std::filesystem;

namespace ioc {
namespace gaia {

MultiFileCatalogV2::MultiFileCatalogV2(const std::string& catalog_dir) 
    : catalog_dir_(catalog_dir) {
    
    if (!fs::exists(catalog_dir_)) {
        throw std::runtime_error("Catalog directory not found: " + catalog_dir_);
    }
    
    if (!loadMetadata()) {
        throw std::runtime_error("Failed to load catalog metadata");
    }
}

MultiFileCatalogV2::~MultiFileCatalogV2() = default;

bool MultiFileCatalogV2::loadMetadata() {
    std::string metadata_path = catalog_dir_ + "/metadata.dat";
    std::ifstream file(metadata_path, std::ios::binary);
    
    if (!file) {
        return false;
    }
    
    // Read header
    file.read(reinterpret_cast<char*>(&header_), sizeof(header_));
    if (!file || std::string(header_.magic, 8) != "GAIA18V2") {
        return false;
    }
    
    // Read HEALPix index
    healpix_index_.resize(header_.num_healpix_pixels);
    file.read(reinterpret_cast<char*>(healpix_index_.data()), 
             header_.num_healpix_pixels * sizeof(HEALPixIndexEntry));
    
    // Read chunk index
    chunk_index_.resize(header_.total_chunks);
    file.read(reinterpret_cast<char*>(chunk_index_.data()),
             header_.total_chunks * sizeof(ChunkInfo));
    
    return file.good();
}

std::string MultiFileCatalogV2::getChunkPath(uint64_t chunk_id) const {
    // Format: chunks/chunk_000.dat, chunk_001.dat, etc.
    std::string filename = "chunk_" + 
                          std::string(3 - std::to_string(chunk_id).length(), '0') + 
                          std::to_string(chunk_id) + ".dat";
    return catalog_dir_ + "/chunks/" + filename;
}

std::vector<Mag18RecordV2> MultiFileCatalogV2::loadChunk(uint64_t chunk_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check cache first
    auto it = chunk_cache_.find(chunk_id);
    if (it != chunk_cache_.end()) {
        it->second->last_access = std::chrono::steady_clock::now();
        cache_hits_++;
        return it->second->records;
    }
    
    cache_misses_++;
    
    // Load from disk
    std::string chunk_path = getChunkPath(chunk_id);
    std::ifstream file(chunk_path, std::ios::binary);
    
    if (!file) {
        std::cerr << "Failed to open chunk file: " << chunk_path << std::endl;
        return {};
    }
    
    // Get file size and calculate number of records
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    size_t num_records = file_size / sizeof(Mag18RecordV2);
    
    // Read all records
    std::vector<Mag18RecordV2> records(num_records);
    file.read(reinterpret_cast<char*>(records.data()), file_size);
    
    if (!file) {
        std::cerr << "Failed to read chunk data: " << chunk_path << std::endl;
        return {};
    }
    
    // Add to cache
    while (chunk_cache_.size() >= MAX_CACHED_CHUNKS) {
        evictLRUChunk();
    }
    
    auto cache_entry = std::make_unique<ChunkCache>(chunk_id, records);
    auto& result = cache_entry->records;
    chunk_cache_[chunk_id] = std::move(cache_entry);
    
    return result;
}

void MultiFileCatalogV2::evictLRUChunk() {
    if (chunk_cache_.empty()) return;
    
    auto lru_it = std::min_element(chunk_cache_.begin(), chunk_cache_.end(),
        [](const auto& a, const auto& b) {
            return a.second->last_access < b.second->last_access;
        });
    
    chunk_cache_.erase(lru_it);
}

uint32_t MultiFileCatalogV2::getHEALPixPixel(double ra, double dec) const {
    // Use the same HEALPix calculation as the original catalog
    double theta = (90.0 - dec) * M_PI / 180.0;  // Colatitude
    double phi = ra * M_PI / 180.0;               // Longitude
    
    return ang2pix_nest(theta, phi);
}

uint32_t MultiFileCatalogV2::ang2pix_nest(double theta, double phi) const {
    // HEALPix ang2pix_nest implementation (matches original catalog)
    const uint32_t nside = header_.healpix_nside;
    const uint32_t nl2 = 2 * nside;
    const uint32_t nl4 = 4 * nside;
    const double z = cos(theta);
    const double za = fabs(z);
    const double TWOPI = 2.0 * M_PI;
    const double HALFPI = M_PI / 2.0;
    
    // Equatorial region
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
                                   (jp >= nl2) ? 4 : 
                                   (jm >= nl2) ? 6 : 8) + (ir - nside);
        
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

std::vector<uint32_t> MultiFileCatalogV2::getPixelsInCone(double ra, double dec, double radius) const {
    std::vector<uint32_t> pixels;
    
    // Simple implementation: check all pixels in bounding box
    // More sophisticated implementation would use proper HEALPix query_disc
    
    const double deg_per_pixel = 180.0 / (4.0 * header_.healpix_nside);  // Rough estimate
    const int pixel_radius = static_cast<int>(radius / deg_per_pixel) + 2;  // Safety margin
    
    const double dec_min = std::max(-90.0, dec - radius);
    const double dec_max = std::min(90.0, dec + radius);
    const double ra_width = radius / std::cos(dec * M_PI / 180.0);
    
    // Sample points in cone region
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
    
    // Remove duplicates and sort
    std::sort(pixels.begin(), pixels.end());
    pixels.erase(std::unique(pixels.begin(), pixels.end()), pixels.end());
    
    return pixels;
}

double MultiFileCatalogV2::angularDistance(double ra1, double dec1, double ra2, double dec2) const {
    const double deg2rad = M_PI / 180.0;
    const double dra = (ra2 - ra1) * deg2rad;
    const double ddec = (dec2 - dec1) * deg2rad;
    const double a = std::sin(ddec/2) * std::sin(ddec/2) + 
                    std::cos(dec1 * deg2rad) * std::cos(dec2 * deg2rad) * 
                    std::sin(dra/2) * std::sin(dra/2);
    return 2.0 * std::atan2(std::sqrt(a), std::sqrt(1-a)) / deg2rad;
}

GaiaStar MultiFileCatalogV2::recordToStar(const Mag18RecordV2& record) const {
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

std::vector<GaiaStar> MultiFileCatalogV2::queryCone(double ra, double dec, double radius, 
                                                    size_t max_results) {
    auto pixels = getPixelsInCone(ra, dec, radius);
    std::vector<GaiaStar> results;
    
    for (uint32_t pixel : pixels) {
        // Find pixel in index
        auto it = std::lower_bound(healpix_index_.begin(), healpix_index_.end(), pixel,
            [](const HEALPixIndexEntry& entry, uint32_t pix) {
                return entry.pixel_id < pix;
            });
        
        if (it == healpix_index_.end() || it->pixel_id != pixel) {
            continue;
        }
        
        // Load chunks containing this pixel's stars
        uint64_t start_idx = it->first_star_idx;
        uint64_t end_idx = start_idx + it->num_stars;
        
        // Find which chunks contain these stars
        for (uint64_t star_idx = start_idx; star_idx < end_idx; ) {
            // Find chunk containing this star
            uint64_t chunk_id = star_idx / header_.stars_per_chunk;
            
            if (chunk_id >= header_.total_chunks) break;
            
            // Load chunk if needed
            auto chunk_records = loadChunk(chunk_id);
            if (chunk_records.empty()) {
                star_idx = (chunk_id + 1) * header_.stars_per_chunk;
                continue;
            }
            
            // Process stars in this chunk
            uint64_t chunk_start = chunk_id * header_.stars_per_chunk;
            uint64_t local_start = (start_idx > chunk_start) ? start_idx - chunk_start : 0;
            uint64_t local_end = std::min(static_cast<uint64_t>(chunk_records.size()),
                                         end_idx - chunk_start);
            
            for (uint64_t local_idx = local_start; local_idx < local_end; ++local_idx) {
                const auto& record = chunk_records[local_idx];
                
                double dist = angularDistance(ra, dec, record.ra, record.dec);
                if (dist <= radius) {
                    results.push_back(recordToStar(record));
                    
                    if (max_results > 0 && results.size() >= max_results) {
                        return results;
                    }
                }
            }
            
            star_idx = (chunk_id + 1) * header_.stars_per_chunk;
        }
    }
    
    return results;
}

std::vector<GaiaStar> MultiFileCatalogV2::queryConeWithMagnitude(double ra, double dec, double radius,
                                                                  double mag_min, double mag_max,
                                                                  size_t max_results) {
    auto results = queryCone(ra, dec, radius, 0);  // Get all, then filter
    
    // Filter by magnitude
    results.erase(std::remove_if(results.begin(), results.end(),
        [mag_min, mag_max](const GaiaStar& star) {
            return star.phot_g_mean_mag < mag_min || star.phot_g_mean_mag > mag_max;
        }), results.end());
    
    if (max_results > 0 && results.size() > max_results) {
        results.resize(max_results);
    }
    
    return results;
}

std::optional<GaiaStar> MultiFileCatalogV2::queryBySourceId(uint64_t source_id) {
    // Binary search would be ideal, but we don't have sorted chunks by source_id
    // For now, use a simple approach - this could be optimized with an index
    
    for (uint64_t chunk_id = 0; chunk_id < header_.total_chunks; ++chunk_id) {
        auto records = loadChunk(chunk_id);
        
        for (const auto& record : records) {
            if (record.source_id == source_id) {
                return recordToStar(record);
            }
        }
    }
    
    return std::nullopt;
}

void MultiFileCatalogV2::preloadPopularRegions() {
    struct PopularRegion {
        std::string name;
        double ra, dec, radius;
    };
    
    std::vector<PopularRegion> regions = {
        {"Pleiades", 56.75, 24.12, 2.0},
        {"Orion", 83.82, -5.39, 2.0},
        {"Galactic Center", 266.417, -29.006, 3.0}
    };
    
    std::cout << "Pre-loading popular regions...\n";
    
    for (const auto& region : regions) {
        auto pixels = getPixelsInCone(region.ra, region.dec, region.radius);
        
        for (uint32_t pixel : pixels) {
            auto it = std::lower_bound(healpix_index_.begin(), healpix_index_.end(), pixel,
                [](const HEALPixIndexEntry& entry, uint32_t pix) {
                    return entry.pixel_id < pix;
                });
            
            if (it != healpix_index_.end() && it->pixel_id == pixel) {
                uint64_t start_idx = it->first_star_idx;
                uint64_t end_idx = start_idx + it->num_stars;
                
                // Pre-load chunks
                for (uint64_t star_idx = start_idx; star_idx < end_idx; star_idx += header_.stars_per_chunk) {
                    uint64_t chunk_id = star_idx / header_.stars_per_chunk;
                    loadChunk(chunk_id);
                }
            }
        }
        
        std::cout << "  ✅ " << region.name << " region cached\n";
    }
}

MultiFileCatalogV2::CacheStats MultiFileCatalogV2::getCacheStats() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    size_t memory_mb = chunk_cache_.size() * sizeof(Mag18RecordV2) * header_.stars_per_chunk / (1024*1024);
    
    return {
        chunk_cache_.size(),
        cache_hits_,
        cache_misses_,
        memory_mb
    };
}

void MultiFileCatalogV2::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    chunk_cache_.clear();
}

} // namespace gaia
} // namespace ioc