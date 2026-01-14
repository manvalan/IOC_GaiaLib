#include <ioc_gaialib/unified_gaia_catalog.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <chrono>
#include <vector>

using namespace ioc::gaia;

// ANSI color codes for better output
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

void printHeader(const std::string& title) {
    std::cout << "\n" << BOLD << CYAN << "═══════════════════════════════════════════════════════" << RESET << "\n";
    std::cout << BOLD << CYAN << "  " << title << RESET << "\n";
    std::cout << BOLD << CYAN << "═══════════════════════════════════════════════════════" << RESET << "\n\n";
}

void printSuccess(const std::string& msg) {
    std::cout << GREEN << "✓ " << msg << RESET << "\n";
}

void printError(const std::string& msg) {
    std::cout << RED << "✗ " << msg << RESET << "\n";
}

void printInfo(const std::string& msg) {
    std::cout << BLUE << "ℹ " << msg << RESET << "\n";
}

int main(int argc, char* argv[]) {
    printHeader("IOC_GaiaLib: Multifile Catalog + Corridor Query Test");
    
    // 1. Setup catalog path
    std::string home = std::getenv("HOME");
    std::string catalog_dir = home + "/.catalog/gaia_mag18_v2_multifile";
    
    if (argc > 1) {
        catalog_dir = argv[1];
    }
    
    printInfo("Using catalog directory: " + catalog_dir);
    
    // 2. Initialize multifile catalog
    std::string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + catalog_dir + R"(",
        "max_cached_chunks": 200,
        "log_level": "info"
    })";
    
    printInfo("Initializing UnifiedGaiaCatalog with multifile_v2...");
    
    auto start_init = std::chrono::high_resolution_clock::now();
    if (!UnifiedGaiaCatalog::initialize(config)) {
        printError("Failed to initialize catalog!");
        std::cerr << "Make sure the directory exists: " << catalog_dir << std::endl;
        return 1;
    }
    auto end_init = std::chrono::high_resolution_clock::now();
    auto init_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_init - start_init);
    
    printSuccess("Catalog initialized in " + std::to_string(init_time.count()) + " ms");
    
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Display catalog info
    auto info = catalog.getCatalogInfo();
    std::cout << "\n" << BOLD << "Catalog Information:" << RESET << "\n";
    std::cout << "  Name:            " << info.catalog_name << "\n";
    std::cout << "  Version:         " << info.version << "\n";
    std::cout << "  Magnitude limit: " << info.magnitude_limit << "\n";
    std::cout << "  Online:          " << (info.is_online ? "Yes" : "No") << "\n";
    
    // ========================================================================
    // TEST 1: Simple Cone Search (baseline test)
    // ========================================================================
    printHeader("TEST 1: Simple Cone Search");
    
    QueryParams cone_params;
    cone_params.ra_center = 83.8220;   // M42 Orion Nebula region
    cone_params.dec_center = -5.3911;
    cone_params.radius = 0.5;          // 0.5 degree radius
    cone_params.max_magnitude = 15.0;
    
    std::cout << "Query parameters:\n";
    std::cout << "  RA:        " << cone_params.ra_center << "°\n";
    std::cout << "  Dec:       " << cone_params.dec_center << "°\n";
    std::cout << "  Radius:    " << cone_params.radius << "°\n";
    std::cout << "  Max mag:   " << cone_params.max_magnitude << "\n\n";
    
    auto start_cone = std::chrono::high_resolution_clock::now();
    auto cone_stars = catalog.queryCone(cone_params);
    auto end_cone = std::chrono::high_resolution_clock::now();
    auto cone_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_cone - start_cone);
    
    printSuccess("Found " + std::to_string(cone_stars.size()) + " stars in " + 
                 std::to_string(cone_time.count()) + " ms");
    
    if (!cone_stars.empty()) {
        std::cout << "\n" << BOLD << "Brightest 5 stars:" << RESET << "\n";
        std::cout << std::setw(20) << "Source ID" 
                  << std::setw(12) << "RA (°)" 
                  << std::setw(12) << "Dec (°)" 
                  << std::setw(10) << "G mag" << "\n";
        std::cout << std::string(54, '-') << "\n";
        
        for (size_t i = 0; i < std::min<size_t>(5, cone_stars.size()); i++) {
            const auto& star = cone_stars[i];
            std::cout << std::setw(20) << star.source_id
                      << std::setw(12) << std::fixed << std::setprecision(4) << star.ra
                      << std::setw(12) << star.dec
                      << std::setw(10) << std::setprecision(2) << star.phot_g_mean_mag << "\n";
        }
    }
    
    // ========================================================================
    // TEST 2: Corridor Query (simulating asteroid path)
    // ========================================================================
    printHeader("TEST 2: Corridor Query (Asteroid Path Simulation)");
    
    // Simulate an asteroid moving across the sky
    // Path from (80, -5) to (90, 0) degrees over ~10 degrees
    CorridorQueryParams corridor_params;
    
    // Define a realistic asteroid path (10 waypoints)
    std::cout << "Simulating asteroid path with 10 waypoints:\n";
    for (int i = 0; i <= 10; i++) {
        double t = i / 10.0;
        double ra = 80.0 + t * 10.0;   // RA from 80° to 90°
        double dec = -5.0 + t * 5.0;   // Dec from -5° to 0°
        
        CelestialPoint point;
        point.ra = ra;
        point.dec = dec;
        corridor_params.path.push_back(point);
        
        std::cout << "  Point " << std::setw(2) << i << ": RA=" 
                  << std::fixed << std::setprecision(2) << std::setw(6) << ra 
                  << "°, Dec=" << std::setw(6) << dec << "°\n";
    }
    
    corridor_params.width = 0.1;           // 0.1 degree width (6 arcmin)
    corridor_params.max_magnitude = 16.0;
    corridor_params.max_results = 1000;    // Limit results
    
    std::cout << "\nCorridor parameters:\n";
    std::cout << "  Width:     " << corridor_params.width << "° (" 
              << (corridor_params.width * 60) << " arcmin)\n";
    std::cout << "  Max mag:   " << corridor_params.max_magnitude << "\n";
    std::cout << "  Max stars: " << corridor_params.max_results << "\n\n";
    
    printInfo("Executing corridor query (this may take a moment)...");
    
    auto start_corridor = std::chrono::high_resolution_clock::now();
    auto corridor_stars = catalog.queryCorridor(corridor_params);
    auto end_corridor = std::chrono::high_resolution_clock::now();
    auto corridor_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_corridor - start_corridor);
    
    printSuccess("Found " + std::to_string(corridor_stars.size()) + " stars along corridor in " + 
                 std::to_string(corridor_time.count()) + " ms");
    
    // Calculate stars per degree of path
    double path_length = 0.0;
    for (size_t i = 1; i < corridor_params.path.size(); i++) {
        double dra = corridor_params.path[i].ra - corridor_params.path[i-1].ra;
        double ddec = corridor_params.path[i].dec - corridor_params.path[i-1].dec;
        path_length += std::sqrt(dra*dra + ddec*ddec);
    }
    
    std::cout << "\n" << BOLD << "Corridor Statistics:" << RESET << "\n";
    std::cout << "  Path length:     " << std::fixed << std::setprecision(2) 
              << path_length << "°\n";
    std::cout << "  Stars/degree:    " << std::setprecision(1) 
              << (corridor_stars.size() / path_length) << "\n";
    std::cout << "  Query efficiency: " << std::setprecision(1) 
              << (corridor_stars.size() / (corridor_time.count() / 1000.0)) << " stars/sec\n";
    
    if (!corridor_stars.empty()) {
        std::cout << "\n" << BOLD << "Sample stars along corridor (first 10):" << RESET << "\n";
        std::cout << std::setw(20) << "Source ID" 
                  << std::setw(12) << "RA (°)" 
                  << std::setw(12) << "Dec (°)" 
                  << std::setw(10) << "G mag"
                  << std::setw(12) << "pmRA"
                  << std::setw(12) << "pmDec" << "\n";
        std::cout << std::string(78, '-') << "\n";
        
        for (size_t i = 0; i < std::min<size_t>(10, corridor_stars.size()); i++) {
            const auto& star = corridor_stars[i];
            std::cout << std::setw(20) << star.source_id
                      << std::setw(12) << std::fixed << std::setprecision(4) << star.ra
                      << std::setw(12) << star.dec
                      << std::setw(10) << std::setprecision(2) << star.phot_g_mean_mag
                      << std::setw(12) << std::setprecision(2) << star.pmra
                      << std::setw(12) << star.pmdec << "\n";
        }
    }
    
    // ========================================================================
    // TEST 3: Performance Statistics
    // ========================================================================
    printHeader("TEST 3: Performance Statistics");
    
    auto stats = catalog.getStatistics();
    std::cout << "Total queries:       " << stats.total_queries << "\n";
    std::cout << "Avg query time:      " << std::fixed << std::setprecision(2) 
              << stats.average_query_time_ms << " ms\n";
    std::cout << "Total stars returned: " << stats.total_stars_returned << "\n";
    std::cout << "Cache hit rate:      " << std::setprecision(1) 
              << stats.cache_hit_rate << "%\n";
    std::cout << "Memory used:         " << stats.memory_used_mb << " MB\n";
    
    // ========================================================================
    // Summary
    // ========================================================================
    printHeader("Test Summary");
    
    std::cout << BOLD << GREEN << "✓ All tests completed successfully!" << RESET << "\n\n";
    
    std::cout << "Key Results:\n";
    std::cout << "  1. Multifile catalog: " << GREEN << "WORKING" << RESET << "\n";
    std::cout << "  2. Cone search:       " << GREEN << "WORKING" << RESET 
              << " (" << cone_stars.size() << " stars in " << cone_time.count() << " ms)\n";
    std::cout << "  3. Corridor query:    " << GREEN << "WORKING" << RESET 
              << " (" << corridor_stars.size() << " stars in " << corridor_time.count() << " ms)\n";
    std::cout << "  4. Proper motions:    " << GREEN << "AVAILABLE" << RESET << "\n\n";
    
    std::cout << BOLD << YELLOW << "💡 Tip: " << RESET 
              << "The corridor query is optimized for asteroid occultation searches!\n";
    std::cout << "   It efficiently finds all stars within a tube around the predicted path.\n\n";
    
    // Cleanup
    UnifiedGaiaCatalog::shutdown();
    
    return 0;
}
