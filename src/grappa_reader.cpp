#include "ioc_gaialib/grappa_reader.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <set>
#include <cmath>
#include <algorithm>

namespace fs = std::filesystem;

namespace ioc {
namespace gaia {

// =============================================================================
// GrappaReader::Impl
// =============================================================================

class GrappaReader::Impl {
public:
    std::string catalog_dir_;
    bool loaded_;
    
    // In-memory index for fast lookups
    std::unordered_map<int64_t, AsteroidData> source_id_index_;
    std::unordered_map<int, AsteroidData> number_index_;
    std::unordered_map<std::string, AsteroidData> designation_index_;
    
    // HEALPix spatial index (NSIDE=32, same as catalog_compiler)
    int nside_;
    std::unordered_map<int, std::vector<int64_t>> healpix_index_;
    
    // Statistics
    Statistics stats_;
    
    Impl(const std::string& catalog_dir)
        : catalog_dir_(catalog_dir), loaded_(false), nside_(32) {
        
        if (!fs::exists(catalog_dir_)) {
            throw GaiaException(ErrorCode::NETWORK_ERROR,
                              "GRAPPA3E catalog directory not found: " + catalog_dir_);
        }
    }
    
    void loadIndex() {
        if (loaded_) return;
        
        // TODO: Implement actual file parsing based on GRAPPA3E format
        // For now, this is a placeholder structure
        
        // GRAPPA3E uses HEALPix organization, similar to our catalog_compiler
        // We need to:
        // 1. Read the extracted .7z files
        // 2. Parse the binary/text format
        // 3. Build indices
        
        // Scan for data files
        std::vector<fs::path> data_files;
        for (const auto& entry : fs::recursive_directory_iterator(catalog_dir_)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                // Look for common data file extensions
                if (ext == ".dat" || ext == ".bin" || ext == ".txt" || ext == ".csv") {
                    data_files.push_back(entry.path());
                }
            }
        }
        
        if (data_files.empty()) {
            throw GaiaException(ErrorCode::NETWORK_ERROR,
                              "No GRAPPA3E data files found in: " + catalog_dir_ +
                              "\nPlease extract the .7z archives first.");
        }
        
        // Initialize statistics
        stats_ = Statistics();
        stats_.total_asteroids = 0;
        stats_.numbered_asteroids = 0;
        stats_.with_physical_params = 0;
        stats_.with_rotation_data = 0;
        stats_.h_mag_min = 999.0;
        stats_.h_mag_max = -999.0;
        stats_.diameter_min_km = 999999.0;
        stats_.diameter_max_km = 0.0;
        
        // Parse each data file
        for (const auto& file : data_files) {
            parseDataFile(file);
        }
        
        // Build HEALPix spatial index
        buildHEALPixIndex();
        
        loaded_ = true;
    }
    
    void parseDataFile(const fs::path& file_path) {
        // TODO: Implement actual parsing based on GRAPPA3E format
        // This is a placeholder that needs to be completed after reading the PDF
        
        std::ifstream ifs(file_path);
        if (!ifs.is_open()) {
            return;
        }
        
        // Example: If files are CSV format
        std::string line;
        bool first_line = true;
        
        while (std::getline(ifs, line)) {
            if (first_line) {
                first_line = false;
                continue; // Skip header
            }
            
            // Parse line according to actual format
            // AsteroidData asteroid = parseLineFormat(line);
            // addToIndices(asteroid);
        }
    }
    
    void addToIndices(const AsteroidData& asteroid) {
        // Add to source ID index
        source_id_index_[asteroid.gaia_source_id] = asteroid;
        
        // Add to number index if numbered
        if (asteroid.number > 0) {
            number_index_[asteroid.number] = asteroid;
            stats_.numbered_asteroids++;
        }
        
        // Add to designation index
        if (!asteroid.designation.empty()) {
            designation_index_[asteroid.designation] = asteroid;
        }
        
        // Update statistics
        stats_.total_asteroids++;
        if (asteroid.has_physical) stats_.with_physical_params++;
        if (asteroid.has_rotation) stats_.with_rotation_data++;
        
        if (asteroid.h_mag < stats_.h_mag_min) stats_.h_mag_min = asteroid.h_mag;
        if (asteroid.h_mag > stats_.h_mag_max) stats_.h_mag_max = asteroid.h_mag;
        
        if (asteroid.diameter_km > 0) {
            if (asteroid.diameter_km < stats_.diameter_min_km) 
                stats_.diameter_min_km = asteroid.diameter_km;
            if (asteroid.diameter_km > stats_.diameter_max_km) 
                stats_.diameter_max_km = asteroid.diameter_km;
        }
    }
    
    void buildHEALPixIndex() {
        // Build HEALPix spatial index for fast cone searches
        for (const auto& [source_id, asteroid] : source_id_index_) {
            int tile_id = coordinatesToHEALPix(asteroid.ra, asteroid.dec);
            healpix_index_[tile_id].push_back(source_id);
        }
    }
    
    int coordinatesToHEALPix(double ra, double dec) {
        // Convert RA/Dec to HEALPix tile ID (NSIDE=32)
        // Simplified version - proper implementation uses HEALPix library
        
        // Convert to theta (colatitude) and phi (longitude)
        double theta = (90.0 - dec) * M_PI / 180.0;  // [0, pi]
        double phi = ra * M_PI / 180.0;              // [0, 2*pi]
        
        // Simplified HEALPix formula (ring scheme)
        int npix = 12 * nside_ * nside_;
        double z = std::cos(theta);
        double za = std::abs(z);
        
        int tile_id;
        if (za <= 2.0/3.0) {
            // Equatorial region
            double temp1 = nside_ * (0.5 + phi / M_PI);
            double temp2 = nside_ * z * 0.75;
            int jp = (int)(temp1 - temp2);
            int jm = (int)(temp1 + temp2);
            int ir = nside_ + 1 + jp - jm;
            int kshift = 1 - (ir & 1);
            int ip = (jp + jm - nside_ + kshift + 1) / 2;
            ip = ip % (4 * nside_);
            tile_id = 2 * nside_ * (nside_ - 1) + (ir - 1) * 4 * nside_ + ip;
        } else {
            // Polar caps
            double tp = phi / (M_PI / 2.0);
            double tmp = nside_ * std::sqrt(3.0 * (1.0 - za));
            int jp = (int)(tp * tmp);
            int jm = (int)((4.0 - tp) * tmp);
            int ir = jp + jm + 1;
            int ip = (int)(tp * ir);
            ip = ip % (4 * ir);
            
            if (z > 0) {
                tile_id = 2 * ir * (ir - 1) + ip;
            } else {
                tile_id = npix - 2 * ir * (ir + 1) + ip;
            }
        }
        
        return std::max(0, std::min(tile_id, npix - 1));
    }
    
    std::vector<int> findTilesInCone(double ra, double dec, double radius) {
        // Find all HEALPix tiles that intersect with cone
        // Simplified: check tiles in a box around the cone
        std::vector<int> tiles;
        
        double ra_min = ra - radius;
        double ra_max = ra + radius;
        double dec_min = std::max(-90.0, dec - radius);
        double dec_max = std::min(90.0, dec + radius);
        
        // Sample points in the box and collect unique tiles
        std::set<int> tile_set;
        int nsamples = 20;
        
        for (int i = 0; i < nsamples; ++i) {
            double test_ra = ra_min + (ra_max - ra_min) * i / (nsamples - 1);
            for (int j = 0; j < nsamples; ++j) {
                double test_dec = dec_min + (dec_max - dec_min) * j / (nsamples - 1);
                int tile = coordinatesToHEALPix(test_ra, test_dec);
                tile_set.insert(tile);
            }
        }
        
        tiles.assign(tile_set.begin(), tile_set.end());
        return tiles;
    }
    
    double angularDistance(double ra1, double dec1, double ra2, double dec2) {
        // Haversine formula
        double dra = (ra2 - ra1) * M_PI / 180.0;
        double ddec = (dec2 - dec1) * M_PI / 180.0;
        dec1 = dec1 * M_PI / 180.0;
        dec2 = dec2 * M_PI / 180.0;
        
        double a = std::sin(ddec / 2.0) * std::sin(ddec / 2.0) +
                   std::cos(dec1) * std::cos(dec2) *
                   std::sin(dra / 2.0) * std::sin(dra / 2.0);
        
        return 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a)) * 180.0 / M_PI;
    }
};

// =============================================================================
// GrappaReader Public API
// =============================================================================

GrappaReader::GrappaReader(const std::string& catalog_dir)
    : pImpl_(std::make_unique<Impl>(catalog_dir)) {}

GrappaReader::~GrappaReader() = default;

GrappaReader::GrappaReader(GrappaReader&&) noexcept = default;
GrappaReader& GrappaReader::operator=(GrappaReader&&) noexcept = default;

void GrappaReader::loadIndex() {
    pImpl_->loadIndex();
}

std::optional<AsteroidData> GrappaReader::queryBySourceId(int64_t source_id) {
    if (!pImpl_->loaded_) {
        loadIndex();
    }
    
    auto it = pImpl_->source_id_index_.find(source_id);
    if (it != pImpl_->source_id_index_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<AsteroidData> GrappaReader::queryByNumber(int number) {
    if (!pImpl_->loaded_) {
        loadIndex();
    }
    
    auto it = pImpl_->number_index_.find(number);
    if (it != pImpl_->number_index_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<AsteroidData> GrappaReader::queryByDesignation(const std::string& designation) {
    if (!pImpl_->loaded_) {
        loadIndex();
    }
    
    auto it = pImpl_->designation_index_.find(designation);
    if (it != pImpl_->designation_index_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<AsteroidData> GrappaReader::queryCone(double ra, double dec, double radius) {
    if (!pImpl_->loaded_) {
        loadIndex();
    }
    
    std::vector<AsteroidData> results;
    
    // Find relevant HEALPix tiles
    auto tiles = pImpl_->findTilesInCone(ra, dec, radius);
    
    // Check asteroids in those tiles
    for (int tile_id : tiles) {
        auto it = pImpl_->healpix_index_.find(tile_id);
        if (it != pImpl_->healpix_index_.end()) {
            for (int64_t source_id : it->second) {
                const auto& asteroid = pImpl_->source_id_index_[source_id];
                double dist = pImpl_->angularDistance(ra, dec, asteroid.ra, asteroid.dec);
                if (dist <= radius) {
                    results.push_back(asteroid);
                }
            }
        }
    }
    
    return results;
}

std::vector<AsteroidData> GrappaReader::query(const AsteroidQuery& query) {
    if (!pImpl_->loaded_) {
        loadIndex();
    }
    
    std::vector<AsteroidData> results;
    
    for (const auto& [source_id, asteroid] : pImpl_->source_id_index_) {
        bool matches = true;
        
        // Check spatial constraints
        if (query.ra_min && asteroid.ra < *query.ra_min) matches = false;
        if (query.ra_max && asteroid.ra > *query.ra_max) matches = false;
        if (query.dec_min && asteroid.dec < *query.dec_min) matches = false;
        if (query.dec_max && asteroid.dec > *query.dec_max) matches = false;
        
        // Check magnitude constraints
        if (query.h_mag_min && asteroid.h_mag < *query.h_mag_min) matches = false;
        if (query.h_mag_max && asteroid.h_mag > *query.h_mag_max) matches = false;
        if (query.g_mag_min && asteroid.phot_g_mean_mag < *query.g_mag_min) matches = false;
        if (query.g_mag_max && asteroid.phot_g_mean_mag > *query.g_mag_max) matches = false;
        
        // Check physical constraints
        if (query.diameter_min_km && asteroid.diameter_km < *query.diameter_min_km) matches = false;
        if (query.diameter_max_km && asteroid.diameter_km > *query.diameter_max_km) matches = false;
        if (query.albedo_min && asteroid.albedo < *query.albedo_min) matches = false;
        if (query.albedo_max && asteroid.albedo > *query.albedo_max) matches = false;
        
        // Check filter flags
        if (query.only_numbered && asteroid.number == 0) matches = false;
        if (query.require_physical && !asteroid.has_physical) matches = false;
        if (query.require_rotation && !asteroid.has_rotation) matches = false;
        
        if (matches) {
            results.push_back(asteroid);
        }
    }
    
    return results;
}

size_t GrappaReader::getCount() const {
    return pImpl_->stats_.total_asteroids;
}

GrappaReader::Statistics GrappaReader::getStatistics() const {
    return pImpl_->stats_;
}

bool GrappaReader::isLoaded() const {
    return pImpl_->loaded_;
}

// =============================================================================
// GaiaAsteroidMatcher
// =============================================================================

GaiaAsteroidMatcher::GaiaAsteroidMatcher(GrappaReader* grappa_reader)
    : reader_(grappa_reader) {}

bool GaiaAsteroidMatcher::isAsteroid(int64_t source_id) {
    return reader_->queryBySourceId(source_id).has_value();
}

std::optional<AsteroidData> GaiaAsteroidMatcher::getAsteroidData(int64_t source_id) {
    return reader_->queryBySourceId(source_id);
}

bool GaiaAsteroidMatcher::enrichStar(GaiaStar& star) {
    auto asteroid = reader_->queryBySourceId(star.source_id);
    if (asteroid) {
        // Add asteroid information to star designation
        // Could add a custom field to GaiaStar for this
        return true;
    }
    return false;
}

} // namespace gaia
} // namespace ioc
