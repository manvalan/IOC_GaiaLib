#include "ioc_gaialib/gaia_local_catalog.h"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <set>
#include <stdexcept>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

namespace ioc_gaialib {

// ============================================================================
// GrappaRecord implementation - Parse 65-byte GRAPPA3E format
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
    return read_le<uint64_t>(raw_data, 0);
}

double GrappaRecord::getRA() const {
    // Bytes 8-11: RA truncated to 1mas (int32)
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
    
    // Errors
    result.parallax_error = read_le<uint16_t>(raw_data, 24) * 0.01; // 10µas to mas
    result.pmra_error = read_le<uint16_t>(raw_data, 30) / 1000.0; // µas/yr to mas/yr
    result.pmdec_error = read_le<uint16_t>(raw_data, 36) / 1000.0;
    
    // Magnitudes
    if (auto g = getMagG()) result.phot_g_mean_mag = *g;
    if (auto bp = getMagBP()) result.phot_bp_mean_mag = *bp;
    if (auto rp = getMagRP()) result.phot_rp_mean_mag = *rp;
    
    return result;
}

// ============================================================================
// GaiaLocalCatalog::Impl - Internal implementation
// ============================================================================

class GaiaLocalCatalog::Impl {
public:
    fs::path catalog_path_;
    mutable GaiaLocalStats stats_;
    bool is_loaded_{false};
    
    // GRAPPA3E tile structure
    struct TileCoord {
        int ra_band;  // 0..359
        int dec_band; // 5..174 (or special: -1=S85-90, 999=N85-90)
        
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
    
    Impl(const std::string& path) : catalog_path_(path) {
        if (!fs::exists(catalog_path_)) {
            throw std::runtime_error("GRAPPA3E catalog not found: " + path);
        }
        is_loaded_ = true;
    }
    
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
        tile.ra_band = static_cast<int>(std::floor(ra_deg));
        if (tile.ra_band < 0) tile.ra_band += 360;
        if (tile.ra_band >= 360) tile.ra_band -= 360;
        
        tile.dec_band = static_cast<int>(std::floor(dec_deg + 90.0));
        if (tile.dec_band < 5) tile.dec_band = 5;
        if (tile.dec_band > 174) tile.dec_band = 174;
        
        return tile;
    }
    
    std::set<TileCoord> getTilesInCone(double ra_deg, double dec_deg, double radius_deg) const {
        std::set<TileCoord> tiles;
        
        // Add center tile
        tiles.insert(getTileCoord(ra_deg, dec_deg));
        
        // Sample perimeter of cone
        int n_samples = std::max(16, static_cast<int>(std::ceil(radius_deg * 20)));
        for (int i = 0; i < n_samples; ++i) {
            double angle = 2.0 * M_PI * i / n_samples;
            double ra_offset = radius_deg * std::cos(angle);
            double dec_offset = radius_deg * std::sin(angle);
            
            // Account for RA compression near poles
            if (std::abs(dec_deg) < 89.0) {
                ra_offset /= std::cos(dec_deg * M_PI / 180.0);
            }
            
            double sample_ra = ra_deg + ra_offset;
            double sample_dec = dec_deg + dec_offset;
            
            // Wrap RA
            while (sample_ra < 0.0) sample_ra += 360.0;
            while (sample_ra >= 360.0) sample_ra -= 360.0;
            
            // Clamp Dec
            if (sample_dec < -90.0) sample_dec = -90.0;
            if (sample_dec > 90.0) sample_dec = 90.0;
            
            tiles.insert(getTileCoord(sample_ra, sample_dec));
        }
        
        // For large cones, add interior points
        if (radius_deg > 1.5) {
            int n_rings = static_cast<int>(std::ceil(radius_deg));
            for (int ring = 1; ring <= n_rings; ++ring) {
                double r = ring;
                if (r > radius_deg) r = radius_deg;
                
                int ring_samples = std::max(8, static_cast<int>(r * 10));
                for (int i = 0; i < ring_samples; ++i) {
                    double angle = 2.0 * M_PI * i / ring_samples;
                    double ra_offset = r * std::cos(angle);
                    double dec_offset = r * std::sin(angle);
                    
                    if (std::abs(dec_deg) < 89.0) {
                        ra_offset /= std::cos(dec_deg * M_PI / 180.0);
                    }
                    
                    double sample_ra = ra_deg + ra_offset;
                    double sample_dec = dec_deg + dec_offset;
                    
                    while (sample_ra < 0.0) sample_ra += 360.0;
                    while (sample_ra >= 360.0) sample_ra -= 360.0;
                    if (sample_dec < -90.0) sample_dec = -90.0;
                    if (sample_dec > 90.0) sample_dec = 90.0;
                    
                    tiles.insert(getTileCoord(sample_ra, sample_dec));
                }
            }
        }
        
        return tiles;
    }
    
    std::vector<GaiaStar> loadTile(const TileCoord& tile) const {
        std::vector<GaiaStar> stars;
        
        // Build file path: catalog_path/dec_dir/ra-dec
        fs::path tile_dir = catalog_path_ / tile.getDirName();
        fs::path tile_file = tile_dir / tile.getFileName();
        
        if (!fs::exists(tile_file)) {
            return stars; // Tile doesn't exist
        }
        
        std::ifstream file(tile_file, std::ios::binary);
        if (!file) {
            return stars;
        }
        
        // Read records (52 bytes each as per PDF)
        GrappaRecord record;
        while (file.read(reinterpret_cast<char*>(&record), sizeof(GrappaRecord))) {
            // Skip duplicated sources (flag bit 7 = 0x80)
            // Duplicated records have incorrect coordinates for this tile
            uint8_t flags = record.raw_data[39];
            if ((flags & 0x80) != 0) {
                continue; // Skip duplicated
            }
            
            // Validate coordinates are physically possible
            double ra = record.getRA();
            double dec = record.getDec();
            
            // Skip records with impossible coordinates
            if (dec < -90.0 || dec > 90.0 || ra < 0.0 || ra >= 360.0) {
                continue;
            }
            
            stars.push_back(record.toGaiaStar());
        }
        
        return stars;
    }
    
    // Angular distance in degrees
    double angularDistance(double ra1, double dec1, double ra2, double dec2) const {
        double d_ra = (ra2 - ra1) * M_PI / 180.0;
        double d_dec = (dec2 - dec1) * M_PI / 180.0;
        dec1 *= M_PI / 180.0;
        dec2 *= M_PI / 180.0;
        
        double a = std::sin(d_dec / 2.0) * std::sin(d_dec / 2.0) +
                   std::cos(dec1) * std::cos(dec2) *
                   std::sin(d_ra / 2.0) * std::sin(d_ra / 2.0);
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
        return c * 180.0 / M_PI;
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
    
    // Get all tiles intersecting the cone
    auto tiles = pImpl->getTilesInCone(ra_deg, dec_deg, radius_deg);
    
    // Load stars from each tile and filter by distance
    for (const auto& tile : tiles) {
        auto tile_stars = pImpl->loadTile(tile);
        
        for (const auto& star : tile_stars) {
            double dist = pImpl->angularDistance(ra_deg, dec_deg, star.ra, star.dec);
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
    
    // Brute force: search all tiles
    // TODO: Could optimize with source_id index
    
    for (int dec_band = 5; dec_band <= 174; ++dec_band) {
        Impl::TileCoord tile;
        tile.dec_band = dec_band;
        
        for (int ra_band = 0; ra_band < 360; ++ra_band) {
            tile.ra_band = ra_band;
            
            auto stars = pImpl->loadTile(tile);
            for (const auto& star : stars) {
                if (star.source_id == source_id) {
                    return star;
                }
            }
        }
    }
    
    // Check polar caps
    Impl::TileCoord north_pole{0, 999};
    auto north_stars = pImpl->loadTile(north_pole);
    for (const auto& star : north_stars) {
        if (star.source_id == source_id) {
            return star;
        }
    }
    
    Impl::TileCoord south_pole{0, -1};
    auto south_stars = pImpl->loadTile(south_pole);
    for (const auto& star : south_stars) {
        if (star.source_id == source_id) {
            return star;
        }
    }
    
    return std::nullopt;
}

GaiaLocalStats GaiaLocalCatalog::getStatistics() const {
    return pImpl->stats_;
}

bool GaiaLocalCatalog::isLoaded() const {
    return pImpl && pImpl->is_loaded_;
}

std::string GaiaLocalCatalog::getCatalogPath() const {
    return pImpl->catalog_path_.string();
}

std::vector<GaiaStar> GaiaLocalCatalog::queryConeWithMagnitude(
    double ra_deg,
    double dec_deg,
    double radius_deg,
    double mag_min,
    double mag_max,
    size_t max_results
) const {
    auto all_stars = queryCone(ra_deg, dec_deg, radius_deg, 0);
    
    std::vector<GaiaStar> filtered;
    for (const auto& star : all_stars) {
        if (star.phot_g_mean_mag >= mag_min && star.phot_g_mean_mag <= mag_max) {
            filtered.push_back(star);
            if (max_results > 0 && filtered.size() >= max_results) {
                break;
            }
        }
    }
    
    return filtered;
}

std::vector<GaiaStar> GaiaLocalCatalog::queryBrightest(
    double ra_deg,
    double dec_deg,
    double radius_deg,
    size_t n_brightest
) const {
    auto all_stars = queryCone(ra_deg, dec_deg, radius_deg, 0);
    
    // Sort by magnitude (brighter = smaller mag)
    std::sort(all_stars.begin(), all_stars.end(),
              [](const GaiaStar& a, const GaiaStar& b) {
                  return a.phot_g_mean_mag < b.phot_g_mean_mag;
              });
    
    if (all_stars.size() > n_brightest) {
        all_stars.resize(n_brightest);
    }
    
    return all_stars;
}

std::vector<GaiaStar> GaiaLocalCatalog::queryBox(
    double ra_min,
    double ra_max,
    double dec_min,
    double dec_max,
    size_t max_results
) const {
    if (!pImpl->is_loaded_) {
        throw std::runtime_error("Catalog not loaded");
    }
    
    std::vector<GaiaStar> results;
    
    // Handle RA wrap-around at 0/360
    bool ra_wraps = ra_max < ra_min;
    
    // Determine Dec band range
    int dec_band_min = static_cast<int>(std::floor(dec_min + 90.0));
    int dec_band_max = static_cast<int>(std::floor(dec_max + 90.0));
    
    if (dec_band_min < 5) dec_band_min = 5;
    if (dec_band_max > 174) dec_band_max = 174;
    
    // Iterate through relevant tiles
    for (int dec_band = dec_band_min; dec_band <= dec_band_max; ++dec_band) {
        int ra_band_min = static_cast<int>(std::floor(ra_min));
        int ra_band_max = static_cast<int>(std::floor(ra_max));
        
        if (ra_wraps) {
            // Handle wrap: [ra_min, 360) and [0, ra_max]
            for (int ra_band = ra_band_min; ra_band < 360; ++ra_band) {
                Impl::TileCoord tile{ra_band, dec_band};
                auto tile_stars = pImpl->loadTile(tile);
                
                for (const auto& star : tile_stars) {
                    if (star.dec >= dec_min && star.dec <= dec_max &&
                        star.ra >= ra_min) {
                        results.push_back(star);
                        if (max_results > 0 && results.size() >= max_results) {
                            return results;
                        }
                    }
                }
            }
            
            for (int ra_band = 0; ra_band <= ra_band_max; ++ra_band) {
                Impl::TileCoord tile{ra_band, dec_band};
                auto tile_stars = pImpl->loadTile(tile);
                
                for (const auto& star : tile_stars) {
                    if (star.dec >= dec_min && star.dec <= dec_max &&
                        star.ra <= ra_max) {
                        results.push_back(star);
                        if (max_results > 0 && results.size() >= max_results) {
                            return results;
                        }
                    }
                }
            }
        } else {
            // Normal case: no wrap
            for (int ra_band = ra_band_min; ra_band <= ra_band_max; ++ra_band) {
                Impl::TileCoord tile{ra_band, dec_band};
                auto tile_stars = pImpl->loadTile(tile);
                
                for (const auto& star : tile_stars) {
                    if (star.ra >= ra_min && star.ra <= ra_max &&
                        star.dec >= dec_min && star.dec <= dec_max) {
                        results.push_back(star);
                        if (max_results > 0 && results.size() >= max_results) {
                            return results;
                        }
                    }
                }
            }
        }
    }
    
    return results;
}

size_t GaiaLocalCatalog::countInCone(
    double ra_deg,
    double dec_deg,
    double radius_deg
) const {
    auto stars = queryCone(ra_deg, dec_deg, radius_deg, 0);
    return stars.size();
}

} // namespace ioc_gaialib
