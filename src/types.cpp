#include "ioc_gaialib/types.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace ioc {
namespace gaia {

// =============================================================================
// GaiaStar Implementation
// =============================================================================

GaiaStar::GaiaStar()
    : source_id(0),
      ra(0.0), dec(0.0),
      parallax(0.0), parallax_error(0.0),
      pmra(0.0), pmdec(0.0),
      pmra_error(0.0), pmdec_error(0.0),
      phot_g_mean_mag(0.0), phot_bp_mean_mag(0.0), phot_rp_mean_mag(0.0),
      bp_rp(0.0),
      astrometric_excess_noise(0.0), astrometric_chi2_al(0.0),
      visibility_periods_used(0),
      ruwe(0.0) {}

bool GaiaStar::isValid() const {
    // Check basic validity criteria
    if (source_id <= 0) return false;
    
    // RA range [0, 360)
    if (ra < 0.0 || ra >= 360.0) return false;
    
    // Dec range [-90, 90]
    if (dec < -90.0 || dec > 90.0) return false;
    
    // G magnitude should be reasonable
    if (phot_g_mean_mag < -5.0 || phot_g_mean_mag > 30.0) return false;
    
    // Parallax error should be positive
    if (parallax_error < 0.0) return false;
    
    return true;
}

double GaiaStar::getBpRpColor() const {
    if (std::isnan(phot_bp_mean_mag) || std::isnan(phot_rp_mean_mag)) {
        return std::nan("");
    }
    return phot_bp_mean_mag - phot_rp_mean_mag;
}

std::string GaiaStar::getDesignation() const {
    std::ostringstream oss;
    oss << "Gaia DR3 " << source_id;
    return oss.str();
}

// =============================================================================
// JulianDate Implementation
// =============================================================================

JulianDate JulianDate::fromCalendar(int year, int month, int day, double ut) {
    // Julian Date calculation (Meeus algorithm)
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    
    double jd = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
    jd += (ut / 24.0) - 0.5;  // Add fractional day from UT (JD starts at noon)
    
    return JulianDate(jd);
}

void JulianDate::toCalendar(int& year, int& month, int& day, double& ut) const {
    // Inverse algorithm (JD starts at noon, so add 0.5 for midnight-based calendar)
    double jd_midnight = jd + 0.5;
    int a = static_cast<int>(jd_midnight);
    int b = a + 1537;
    int c = (b - 122.1) / 365.25;
    int d = 365.25 * c;
    int e = (b - d) / 30.6001;
    
    day = b - d - static_cast<int>(30.6001 * e);
    month = e - (e < 14 ? 1 : 13);
    year = c - (month > 2 ? 4716 : 4715);
    
    // Fractional part is UT
    ut = (jd_midnight - a) * 24.0;
    
}

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Convert string to GaiaRelease enum
 */
GaiaRelease stringToRelease(const std::string& str) {
    if (str == "DR2") return GaiaRelease::DR2;
    if (str == "EDR3") return GaiaRelease::EDR3;
    if (str == "DR3") return GaiaRelease::DR3;
    
    throw GaiaException(ErrorCode::INVALID_PARAMS, 
                       "Invalid Gaia release: " + str);
}

/**
 * Convert GaiaRelease enum to string
 */
std::string releaseToString(GaiaRelease release) {
    switch (release) {
        case GaiaRelease::DR2:  return "DR2";
        case GaiaRelease::EDR3: return "EDR3";
        case GaiaRelease::DR3:  return "DR3";
        default: return "UNKNOWN";
    }
}

/**
 * Calculate angular distance between two coordinates (haversine formula)
 * @return Distance in degrees
 */
double angularDistance(const EquatorialCoordinates& c1, 
                      const EquatorialCoordinates& c2) {
    constexpr double DEG_TO_RAD = M_PI / 180.0;
    
    double ra1 = c1.ra * DEG_TO_RAD;
    double dec1 = c1.dec * DEG_TO_RAD;
    double ra2 = c2.ra * DEG_TO_RAD;
    double dec2 = c2.dec * DEG_TO_RAD;
    
    double dra = ra2 - ra1;
    double ddec = dec2 - dec1;
    
    double a = std::sin(ddec / 2.0) * std::sin(ddec / 2.0) +
               std::cos(dec1) * std::cos(dec2) *
               std::sin(dra / 2.0) * std::sin(dra / 2.0);
    
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    
    return c / DEG_TO_RAD;  // Convert back to degrees
}

/**
 * Validate coordinate ranges
 */
bool isValidCoordinate(const EquatorialCoordinates& coord) {
    return (coord.ra >= 0.0 && coord.ra < 360.0 &&
            coord.dec >= -90.0 && coord.dec <= 90.0);
}

/**
 * Validate query parameters
 */
bool isValidQueryParams(const QueryParams& params) {
    if (params.radius <= 0.0 || params.radius > 180.0) return false;
    if (params.max_magnitude < 0.0 || params.max_magnitude > 30.0) return false;
    
    EquatorialCoordinates center(params.ra_center, params.dec_center);
    return isValidCoordinate(center);
}

/**
 * Validate box region
 */
bool isValidBoxRegion(const BoxRegion& box) {
    if (box.ra_min < 0.0 || box.ra_max > 360.0) return false;
    if (box.dec_min < -90.0 || box.dec_max > 90.0) return false;
    if (box.ra_min >= box.ra_max) return false;
    if (box.dec_min >= box.dec_max) return false;
    return true;
}

} // namespace gaia
} // namespace ioc
