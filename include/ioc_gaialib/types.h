#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace ioc {
namespace gaia {

/**
 * Gaia data release versions
 */
enum class GaiaRelease {
    DR2,      ///< Gaia Data Release 2 (2018)
    EDR3,     ///< Gaia Early Data Release 3 (2020)
    DR3       ///< Gaia Data Release 3 (2022)
};

/**
 * Gaia star data structure
 * Contains all essential fields from Gaia DR3
 */
struct GaiaStar {
    int64_t source_id;           ///< Unique Gaia source identifier
    
    // Astrometric data (ICRS, Epoch 2016.0)
    double ra;                   ///< Right Ascension [degrees]
    double dec;                  ///< Declination [degrees]
    double parallax;             ///< Parallax [mas]
    double parallax_error;       ///< Parallax standard error [mas]
    
    // Proper motion
    double pmra;                 ///< Proper motion in RA * cos(dec) [mas/yr]
    double pmdec;                ///< Proper motion in Dec [mas/yr]
    double pmra_error;           ///< PM RA standard error [mas/yr]
    double pmdec_error;          ///< PM Dec standard error [mas/yr]
    
    // Photometry
    double phot_g_mean_mag;      ///< G-band mean magnitude [mag]
    double phot_bp_mean_mag;     ///< BP-band mean magnitude [mag]
    double phot_rp_mean_mag;     ///< RP-band mean magnitude [mag]
    double bp_rp;                ///< BP-RP color [mag]
    
    // Quality flags
    double astrometric_excess_noise;
    double astrometric_chi2_al;
    int visibility_periods_used;
    double ruwe;                 ///< Renormalized Unit Weight Error
    
    // Constructor
    GaiaStar();
    
    // Utility methods
    bool isValid() const;
    double getBpRpColor() const;
    std::string getDesignation() const;
};

/**
 * Equatorial coordinates (J2000.0)
 */
struct EquatorialCoordinates {
    double ra;                   ///< Right Ascension [degrees]
    double dec;                  ///< Declination [degrees]
    
    EquatorialCoordinates() : ra(0.0), dec(0.0) {}
    EquatorialCoordinates(double ra_, double dec_) : ra(ra_), dec(dec_) {}
};

/**
 * Julian Date representation
 */
struct JulianDate {
    double jd;                   ///< Julian Date
    
    JulianDate() : jd(0.0) {}
    explicit JulianDate(double jd_) : jd(jd_) {}
    
    // Convert to/from calendar date
    static JulianDate fromCalendar(int year, int month, int day, double ut);
    void toCalendar(int& year, int& month, int& day, double& ut) const;
    
    // Epoch conversions
    static JulianDate J2000() { return JulianDate(2451545.0); }
    static JulianDate J2016() { return JulianDate(2457389.0); }  // Gaia DR3 epoch
    
    double toYears() const { return (jd - 2451545.0) / 365.25 + 2000.0; }
};

/**
 * Query parameters for Gaia catalog searches
 */
struct QueryParams {
    double ra_center;            ///< Center RA [degrees]
    double dec_center;           ///< Center Dec [degrees]
    double radius;               ///< Search radius [degrees]
    double max_magnitude;        ///< Maximum G magnitude
    double min_parallax;         ///< Minimum parallax [mas], -1 = no limit
    
    QueryParams() 
        : ra_center(0.0), dec_center(0.0), radius(1.0), 
          max_magnitude(20.0), min_parallax(-1.0) {}
};

/**
 * Rectangular search region
 */
struct BoxRegion {
    double ra_min;               ///< Minimum RA [degrees]
    double ra_max;               ///< Maximum RA [degrees]
    double dec_min;              ///< Minimum Dec [degrees]
    double dec_max;              ///< Maximum Dec [degrees]
    
    BoxRegion() : ra_min(0.0), ra_max(0.0), dec_min(0.0), dec_max(0.0) {}
    BoxRegion(double ra_min_, double ra_max_, double dec_min_, double dec_max_)
        : ra_min(ra_min_), ra_max(ra_max_), dec_min(dec_min_), dec_max(dec_max_) {}
};

/**
 * Cache statistics
 */
struct CacheStats {
    size_t total_tiles;          ///< Total HEALPix tiles
    size_t cached_tiles;         ///< Number of cached tiles
    size_t total_stars;          ///< Total cached stars
    size_t disk_size_bytes;      ///< Cache size on disk
    std::chrono::system_clock::time_point last_update;
    
    CacheStats() : total_tiles(0), cached_tiles(0), total_stars(0), disk_size_bytes(0) {}
    
    double getCoveragePercent() const {
        return total_tiles > 0 ? (100.0 * cached_tiles / total_tiles) : 0.0;
    }
};

/**
 * Progress callback for async operations
 */
using ProgressCallback = std::function<void(int percent, const std::string& message)>;

/**
 * Error types for exception handling
 */
enum class ErrorCode {
    SUCCESS = 0,
    NETWORK_ERROR,
    TIMEOUT,
    PARSE_ERROR,
    INVALID_PARAMS,
    CACHE_ERROR,
    RATE_LIMIT_EXCEEDED,
    SERVICE_UNAVAILABLE
};

/**
 * Exception class for Gaia library errors
 */
class GaiaException : public std::exception {
public:
    GaiaException(ErrorCode code, const std::string& message)
        : code_(code), message_(message) {}
    
    const char* what() const noexcept override { return message_.c_str(); }
    ErrorCode code() const noexcept { return code_; }
    
private:
    ErrorCode code_;
    std::string message_;
};

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Convert string to GaiaRelease enum
 */
GaiaRelease stringToRelease(const std::string& str);

/**
 * Convert GaiaRelease enum to string
 */
std::string releaseToString(GaiaRelease release);

/**
 * Calculate angular distance between two coordinates (haversine formula)
 * @return Distance in degrees
 */
double angularDistance(const EquatorialCoordinates& c1, 
                      const EquatorialCoordinates& c2);

/**
 * Validate coordinate ranges
 */
bool isValidCoordinate(const EquatorialCoordinates& coord);

/**
 * Validate query parameters
 */
bool isValidQueryParams(const QueryParams& params);

/**
 * Validate box region
 */
bool isValidBoxRegion(const BoxRegion& box);

} // namespace gaia
} // namespace ioc
