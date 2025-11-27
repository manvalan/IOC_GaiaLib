#include "ioc_gaialib/gaia_local_catalog.h"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <unordered_map>
#include <set>
#include <stdexcept>
#include <sstream>

namespace fs = std::filesystem;

namespace ioc_gaialib {

// ============================================================================
// GrappaRecord implementation - Parse raw 65-byte binary format
// ============================================================================

// Helper to read little-endian integers from byte array
namespace {
    template<typename T>
    T read_le(const uint8_t* data, size_t offset) {
        T value = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            value |= static_cast<T>(data[offset + i]) << (i * 8);
        }
        return value;
    }
    
    int32_t read_int32_le(const uint8_t* data, size_t offset) {
        return static_cast<int32_t>(read_le<uint32_t>(data, offset));
    }
}

uint64_t GrappaRecord::getSourceId() const {
    // First 8 bytes: source_id (little-endian uint64)
    return read_le<uint64_t>(raw_data, 0);
}

double GrappaRecord::getRA() const {
    // Bytes 8-11: RA truncated to 1mas (int32, little-endian)
    // Byte 12: RA fractional component in 4µas units
    int32_t ra_mas = read_int32_le(raw_data, 8);
    uint8_t ra2_4uas = raw_data[12];
    double ra_mas_total = static_cast<double>(ra_mas) + (ra2_4uas * 0.004);
    return ra_mas_total / 3600000.0; // mas to degrees
}

double GrappaRecord::getDec() const {
    // Bytes 15-18: Dec+90° truncated to 1mas (int32)
    // Byte 19: Dec fractional component in 4µas units
    int32_t dec_mas = read_int32_le(raw_data, 15);
    uint8_t dec2_4uas = raw_data[19];
    double dec_mas_total = static_cast<double>(dec_mas) + (dec2_4uas * 0.004);
    return (dec_mas_total / 3600000.0) - 90.0; // mas to degrees, remove offset
}

double GrappaRecord::getParallax() const {
    // Bytes 22-23: Parallax in 12.5µas units (uint16)
    uint16_t parallax_12_5uas = read_le<uint16_t>(raw_data, 22);
    return parallax_12_5uas * 0.0125; // Convert to mas
}

double GrappaRecord::getPMRA() const {
    // Bytes 26-29: PM_RA * cos(dec) in µas/yr (int32)
    int32_t pmra_uas_yr = read_int32_le(raw_data, 26);
    return static_cast<double>(pmra_uas_yr) / 1000.0; // µas/yr to mas/yr
}

double GrappaRecord::getPMDec() const {
    // Bytes 32-35: PM_Dec in µas/yr (int32)
    int32_t pmdec_uas_yr = read_int32_le(raw_data, 32);
    return static_cast<double>(pmdec_uas_yr) / 1000.0; // µas/yr to mas/yr
}

double GrappaRecord::getRUWE() const {
    // Byte 38: RUWE * 10
    uint8_t ruwe_0_1 = raw_data[38];
    return ruwe_0_1 / 10.0;
}

std::optional<double> GrappaRecord::getMagG() const {
    // Bytes 40-43: G magnitude compressed (uint32)
    // Bit 4 of byte 39 (flags) indicates if G is present
    uint8_t flags = raw_data[39];
    if ((flags & 0x10) == 0) return std::nullopt;
    
    uint32_t g_compressed = read_le<uint32_t>(raw_data, 40);
    // Extract 18-bit magnitude value (LOWER 18 bits)
    // Upper 14 bits contain error (not used here)
    uint32_t mag_value = g_compressed & 0x3FFFF;
    return mag_value / 10000.0; // 0.1 mmag to mag
}

std::optional<double> GrappaRecord::getMagBP() const {
    // Bytes 44-47: BP magnitude compressed
    uint8_t flags = raw_data[39];
    if ((flags & 0x20) == 0) return std::nullopt;
    
    uint32_t bp_compressed = read_le<uint32_t>(raw_data, 44);
    uint32_t mag_value = bp_compressed & 0x3FFFF;
    return mag_value / 10000.0;
}

std::optional<double> GrappaRecord::getMagRP() const {
    // Bytes 48-51: RP magnitude compressed
    uint8_t flags = raw_data[39];
    if ((flags & 0x40) == 0) return std::nullopt;
    
    uint32_t rp_compressed = read_le<uint32_t>(raw_data, 48);
    uint32_t mag_value = rp_compressed & 0x3FFFF;
    return mag_value / 10000.0;
}

GaiaStar GrappaRecord::toGaiaStar() const {
    GaiaStar result;
    result.source_id = getSourceId();
    result.ra = getRA();
    result.dec = getDec();
    result.parallax = getParallax();
    result.pmra = getPMRA();
    result.pmdec = getPMDec();
    
    // Errors (from known byte positions)
    // Note: GaiaStar doesn't have ra_error/dec_error fields, only parallax and PM errors
    result.parallax_error = read_le<uint16_t>(raw_data, 24) * 0.01;
    result.pmra_error = read_le<uint16_t>(raw_data, 30) / 1000.0; // µas/yr to mas/yr
    result.pmdec_error = read_le<uint16_t>(raw_data, 36) / 1000.0;
    
    // Magnitudes
    if (auto g = getMagG()) result.phot_g_mean_mag = *g;
    if (auto bp = getMagBP()) result.phot_bp_mean_mag = *bp;
    if (auto rp = getMagRP()) result.phot_rp_mean_mag = *rp;
    
    return result;
}

// ============================================================================
// GaiaLocalCatalog::Impl
// ============================================================================

class GaiaLocalCatalog::Impl {
public:
    fs::path catalog_path_;
    mutable GaiaLocalStats stats_;
    bool is_loaded_{false};
    
    Impl(const std::string& path) : catalog_path_(path) {
        if (!fs::exists(catalog_path_)) {
            throw std::runtime_error("GRAPPA3E catalog not found: " + path);
        }
        is_loaded_ = true;
    }
    
    // GRAPPA3E structure:
    // - Directory: dec_band = int(dec + 90), range 5..174 (plus N85-90, S85-90)
    // - File: "ra_band-dec_band" where ra_band = int(ra), range 0..359
    
    struct TileCoord {
        int ra_band;  // 0..359
        int dec_band; // 5..174 (or special polar: -1=S85-90, 999=N85-90)
        
        bool operator<(const TileCoord& other) const {
            if (dec_band != other.dec_band) return dec_band < other.dec_band;
            return ra_band < other.ra_band;
        }
        
        std::string getDirName() const {
            if (dec_band == -1) return "S85-90";
            if (dec_band == 999) return "N85-90";
            return std::to_string(dec_band);
        }
        
        std::string getFileName() const {
            if (dec_band == -1 || dec_band == 999) {
                return getDirName();  // Polar files
            }
            return std::to_string(ra_band) + "-" + std::to_string(dec_band);
        }
    };
    
    TileCoord getTileCoord(double ra_deg, double dec_deg) const {
        TileCoord tile;
        
        // Handle polar caps
        if (dec_deg >= 85.0) {
            tile.dec_band = 999;  // N85-90
            tile.ra_band = 0;
            return tile;
        }
        if (dec_deg <= -85.0) {
            tile.dec_band = -1;  // S85-90
            tile.ra_band = 0;
            return tile;
        }
        
        // Normal bands
        tile.ra_band = static_cast<int>(std::floor(ra_deg)) % 360;
        if (tile.ra_band < 0) tile.ra_band += 360;
        
        tile.dec_band = static_cast<int>(std::floor(dec_deg + 90.0));
        if (tile.dec_band < 5) tile.dec_band = 5;
        if (tile.dec_band > 174) tile.dec_band = 174;
        
        return tile;
    }
    
    std::set<TileCoord> getTilesInCone(double ra_deg, double dec_deg, double radius_deg) const {
        std::set<TileCoord> tiles;
        
        // Center tile
        tiles.insert(raDecToHEALPix(ra_deg, dec_deg));
        
        // Sample points around the cone perimeter
        const int n_samples = std::max(8, static_cast<int>(radius_deg * 20));
        for (int i = 0; i < n_samples; ++i) {
            double angle = 2.0 * M_PI * i / n_samples;
            double ra_offset = radius_deg * std::cos(angle) / std::cos(dec_deg * M_PI / 180.0);
            double dec_offset = radius_deg * std::sin(angle);
            
            double sample_ra = ra_deg + ra_offset;
            double sample_dec = dec_deg + dec_offset;
            
            // Normalize RA
            if (sample_ra < 0) sample_ra += 360.0;
            if (sample_ra >= 360.0) sample_ra -= 360.0;
            
            // Clamp Dec
            if (sample_dec < -90.0) sample_dec = -90.0;
            if (sample_dec > 90.0) sample_dec = 90.0;
            
            tiles.insert(raDecToHEALPix(sample_ra, sample_dec));
        }
        
        return tiles;
    }
    
    std::vector<GaiaStar> loadTile(int64_t tile_id) const {
        std::vector<GaiaStar> stars;
        
        // Tile directory structure: catalog_path/XXX/YYY-XXX
        // where XXX is tile_id and YYY is sub-tile (0-359)
        fs::path tile_dir = catalog_path_ / std::to_string(tile_id);
        
        if (!fs::exists(tile_dir) || !fs::is_directory(tile_dir)) {
            return stars; // Tile directory doesn't exist
        }
        
        // Read all sub-tile files in this directory
        for (const auto& entry : fs::directory_iterator(tile_dir)) {
            if (!entry.is_regular_file()) continue;
            
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file) continue;
            
            // Read records
            GrappaRecord record;
            while (file.read(reinterpret_cast<char*>(&record), sizeof(GrappaRecord))) {
                stars.push_back(record.toGaiaStar());
            }
        }
        
        return stars;
    }
};

// ============================================================================
// GaiaLocalCatalog public interface
// ============================================================================

GaiaLocalCatalog::GaiaLocalCatalog(const std::string& catalog_path)
    : pImpl(std::make_unique<Impl>(catalog_path)) {
}

GaiaLocalCatalog::~GaiaLocalCatalog() = default;

GaiaLocalCatalog::GaiaLocalCatalog(GaiaLocalCatalog&&) noexcept = default;
GaiaLocalCatalog& GaiaLocalCatalog::operator=(GaiaLocalCatalog&&) noexcept = default;

std::vector<GaiaStar> GaiaLocalCatalog::queryCone(
    double ra_deg,
    double dec_deg,
    double radius_deg,
    size_t max_results
) const {
    if (!pImpl->is_loaded_) {
        throw std::runtime_error("Catalog not loaded");
    }
    
    std::vector<GaiaStar> results;
    
    // Get tiles that overlap with search cone
    auto tiles = pImpl->getTilesInCone(ra_deg, dec_deg, radius_deg);
    
    // Load and filter stars from each tile
    for (int64_t tile_id : tiles) {
        auto tile_stars = pImpl->loadTile(tile_id);
        
        for (const auto& star : tile_stars) {
            // Angular distance using haversine formula
            EquatorialCoordinates c1{ra_deg, dec_deg};
            EquatorialCoordinates c2{star.ra, star.dec};
            double dist = ioc::gaia::angularDistance(c1, c2);
            if (dist <= radius_deg) {
                results.push_back(star);
                
                if (max_results > 0 && results.size() >= max_results) {
                    return results;
                }
            }
        }
    }
    
    return results;
}

std::optional<GaiaStar> GaiaLocalCatalog::queryBySourceId(uint64_t source_id) const {
    if (!pImpl->is_loaded_) {
        throw std::runtime_error("Catalog not loaded");
    }
    
    // For source_id lookup, we'd need to scan all tiles or build an index
    // This is a simplified implementation that searches likely tiles
    // A production implementation would build a source_id index file
    
    // Search all tile directories
    for (const auto& dir_entry : fs::directory_iterator(pImpl->catalog_path_)) {
        if (!dir_entry.is_directory()) continue;
        
        // Try to parse directory name as tile_id
        std::string dir_name = dir_entry.path().filename().string();
        if (dir_name.find_first_not_of("0123456789") != std::string::npos) {
            continue; // Not a numeric directory
        }
        
        int64_t tile_id = std::stoll(dir_name);
        auto tile_stars = pImpl->loadTile(tile_id);
        
        for (const auto& star : tile_stars) {
            if (star.source_id == source_id) {
                return star;
            }
        }
    }
    
    return std::nullopt;
}

GaiaLocalStats GaiaLocalCatalog::getStatistics() const {
    if (!pImpl->is_loaded_) {
        throw std::runtime_error("Catalog not loaded");
    }
    
    GaiaLocalStats stats;
    
    // Count tiles
    for (const auto& entry : fs::directory_iterator(pImpl->catalog_path_)) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename().string();
            if (name.find_first_not_of("0123456789") == std::string::npos) {
                stats.tiles_loaded++;
            }
        }
    }
    
    return stats;
}

bool GaiaLocalCatalog::isLoaded() const {
    return pImpl && pImpl->is_loaded_;
}

std::string GaiaLocalCatalog::getCatalogPath() const {
    return pImpl->catalog_path_.string();
}

} // namespace ioc_gaialib
