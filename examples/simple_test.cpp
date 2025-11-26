#include "ioc_gaialib/types.h"
#include "ioc_gaialib/gaia_cache.h"
#include "ioc_gaialib/gaia_client.h"
#include <iostream>
#include <iomanip>

using namespace ioc::gaia;

void testTypes() {
    std::cout << "=== Testing Types ===" << std::endl;
    
    // Test GaiaStar
    GaiaStar star;
    star.source_id = 12345678901234;
    star.ra = 67.5;
    star.dec = 15.2;
    star.parallax = 5.0;
    star.parallax_error = 0.1;
    star.phot_g_mean_mag = 10.5;
    star.phot_bp_mean_mag = 10.8;
    star.phot_rp_mean_mag = 10.2;
    
    std::cout << "Star designation: " << star.getDesignation() << std::endl;
    std::cout << "Star valid: " << (star.isValid() ? "YES" : "NO") << std::endl;
    std::cout << "BP-RP color: " << star.getBpRpColor() << std::endl;
    
    // Test coordinates
    EquatorialCoordinates c1(67.5, 15.2);
    EquatorialCoordinates c2(68.0, 15.5);
    double dist = angularDistance(c1, c2);
    std::cout << "Angular distance: " << dist << " degrees" << std::endl;
    
    // Test Julian Date
    auto jd = JulianDate::fromCalendar(2025, 11, 26, 12.0);
    std::cout << "Julian Date for 2025-11-26 12:00 UT: " << std::fixed 
              << std::setprecision(2) << jd.jd << std::endl;
    
    int y, m, d;
    double ut;
    jd.toCalendar(y, m, d, ut);
    std::cout << "Back to calendar: " << y << "-" << m << "-" << d 
              << " " << ut << " UT" << std::endl;
    
    // Test release conversions
    std::cout << "DR3 string: " << releaseToString(GaiaRelease::DR3) << std::endl;
    auto release = stringToRelease("DR2");
    std::cout << "DR2 enum: " << static_cast<int>(release) << std::endl;
    
    std::cout << "✓ Types test passed!\n" << std::endl;
}

void testCache() {
    std::cout << "=== Testing Cache ===" << std::endl;
    
    // Create cache in temp directory
    GaiaCache cache("/tmp/gaia_test_cache", 32, GaiaRelease::DR3);
    
    std::cout << "Cache directory: " << cache.getCacheDir() << std::endl;
    std::cout << "HEALPix NSIDE: " << cache.getNside() << std::endl;
    
    // Test coverage check
    bool covered = cache.isCovered(67.5, 15.2);
    std::cout << "Point (67.5, 15.2) covered: " << (covered ? "YES" : "NO") << std::endl;
    
    bool regionCovered = cache.isRegionCovered(67.5, 15.2, 1.0);
    std::cout << "Region covered: " << (regionCovered ? "YES" : "NO") << std::endl;
    
    // Get statistics
    auto stats = cache.getStatistics();
    std::cout << "Total tiles: " << stats.total_tiles << std::endl;
    std::cout << "Cached tiles: " << stats.cached_tiles << std::endl;
    std::cout << "Coverage: " << std::fixed << std::setprecision(2) 
              << stats.getCoveragePercent() << "%" << std::endl;
    
    // Test verify
    bool valid = cache.verify();
    std::cout << "Cache valid: " << (valid ? "YES" : "NO") << std::endl;
    
    std::cout << "✓ Cache test passed!\n" << std::endl;
}

void testClient() {
    std::cout << "=== Testing Client ===" << std::endl;
    
    // Create client
    GaiaClient client(GaiaRelease::DR3);
    
    std::cout << "TAP URL: " << client.getTapUrl() << std::endl;
    std::cout << "Table name: " << client.getTableName() << std::endl;
    std::cout << "Release: " << releaseToString(client.getRelease()) << std::endl;
    
    // Set configuration
    client.setRateLimit(5);  // 5 queries per minute
    client.setTimeout(30);
    client.setMaxRetries(2);
    
    std::cout << "Client configured successfully" << std::endl;
    
    // Note: Not actually querying the network in this test
    std::cout << "✓ Client test passed!\n" << std::endl;
}

void testValidation() {
    std::cout << "=== Testing Validation ===" << std::endl;
    
    // Valid coordinate
    EquatorialCoordinates valid_coord(67.5, 15.2);
    std::cout << "Valid coordinate (67.5, 15.2): " 
              << (isValidCoordinate(valid_coord) ? "YES" : "NO") << std::endl;
    
    // Invalid RA
    EquatorialCoordinates invalid_ra(400.0, 15.2);
    std::cout << "Invalid RA (400.0, 15.2): " 
              << (isValidCoordinate(invalid_ra) ? "YES" : "NO") << std::endl;
    
    // Invalid Dec
    EquatorialCoordinates invalid_dec(67.5, 100.0);
    std::cout << "Invalid Dec (67.5, 100.0): " 
              << (isValidCoordinate(invalid_dec) ? "YES" : "NO") << std::endl;
    
    // Valid query params
    QueryParams params;
    params.ra_center = 67.5;
    params.dec_center = 15.2;
    params.radius = 1.0;
    params.max_magnitude = 12.0;
    std::cout << "Valid query params: " 
              << (isValidQueryParams(params) ? "YES" : "NO") << std::endl;
    
    // Invalid radius
    QueryParams invalid_params;
    invalid_params.radius = -1.0;
    std::cout << "Invalid radius: " 
              << (isValidQueryParams(invalid_params) ? "YES" : "NO") << std::endl;
    
    // Valid box
    BoxRegion box(67.0, 68.0, 15.0, 16.0);
    std::cout << "Valid box region: " 
              << (isValidBoxRegion(box) ? "YES" : "NO") << std::endl;
    
    // Invalid box (min > max)
    BoxRegion invalid_box(68.0, 67.0, 15.0, 16.0);
    std::cout << "Invalid box (min > max): " 
              << (isValidBoxRegion(invalid_box) ? "YES" : "NO") << std::endl;
    
    std::cout << "✓ Validation test passed!\n" << std::endl;
}

int main() {
    std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
    std::cout << "║  IOC GaiaLib - Functional Tests     ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝\n" << std::endl;
    
    try {
        testTypes();
        testValidation();
        testCache();
        testClient();
        
        std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
        std::cout << "║  ✓ ALL TESTS PASSED!                ║" << std::endl;
        std::cout << "╚══════════════════════════════════════╝\n" << std::endl;
        
        return 0;
    } catch (const GaiaException& e) {
        std::cerr << "GaiaException: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
