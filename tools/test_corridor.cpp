/**
 * Test program for corridor query API
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include "include/ioc_gaialib/unified_gaia_catalog.h"

int main() {
    std::cout << "=== Corridor Query API Test ===" << std::endl;
    
    // Initialize catalog
    std::string catalog_path = std::string(getenv("HOME")) + "/.catalog/gaia_mag18_v2_multifile";
    
    std::cout << "\nLoading catalog from: " << catalog_path << std::endl;
    
    // Create JSON configuration for multifile catalog
    std::string config_json = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + catalog_path + R"(",
        "log_level": "warning"
    })";
    
    if (!ioc::gaia::UnifiedGaiaCatalog::initialize(config_json)) {
        std::cerr << "Failed to initialize catalog!" << std::endl;
        return 1;
    }
    
    auto& catalog = ioc::gaia::UnifiedGaiaCatalog::getInstance();
    auto info = catalog.getCatalogInfo();
    
    std::cout << "Catalog ready with " << info.total_stars << " stars" << std::endl;

    // Test 1: Simple 2-point corridor
    std::cout << "\n=== Test 1: Simple 2-point corridor ===" << std::endl;
    {
        ioc::gaia::CorridorQueryParams params;
        params.path = {
            {70.0, 20.0},  // Start point
            {75.0, 22.0}   // End point
        };
        params.width = 0.2;  // 0.2 degrees half-width
        params.max_magnitude = 15.0;
        params.min_parallax = 0.0;
        params.max_results = 1000;
        
        std::cout << "Path: (70°, 20°) -> (75°, 22°)" << std::endl;
        std::cout << "Width: " << params.width << " degrees" << std::endl;
        std::cout << "Path length: " << params.getPathLength() << " degrees" << std::endl;
        std::cout << "Valid: " << (params.isValid() ? "yes" : "no") << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCorridor(params);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Found " << stars.size() << " stars in " << duration.count() << " ms" << std::endl;
        
        if (stars.size() > 0) {
            std::cout << "First 5 stars:" << std::endl;
            for (size_t i = 0; i < std::min(stars.size(), size_t(5)); i++) {
                std::cout << "  " << stars[i].source_id 
                          << " RA=" << std::fixed << std::setprecision(4) << stars[i].ra
                          << " Dec=" << stars[i].dec
                          << " G=" << std::setprecision(3) << stars[i].phot_g_mean_mag
                          << std::endl;
            }
        }
    }
    
    // Test 2: Multi-point L-shaped corridor 
    std::cout << "\n=== Test 2: L-shaped corridor (3 points) ===" << std::endl;
    {
        ioc::gaia::CorridorQueryParams params;
        params.path = {
            {73.0, 20.0},   // Start
            {73.5, 20.5},   // Middle (turn)
            {74.0, 20.5}    // End
        };
        params.width = 0.15;
        params.max_magnitude = 14.0;
        params.min_parallax = 0.0;
        params.max_results = 500;
        
        std::cout << "Path: (73°, 20°) -> (73.5°, 20.5°) -> (74°, 20.5°)" << std::endl;
        std::cout << "Width: " << params.width << " degrees" << std::endl;
        std::cout << "Path length: " << params.getPathLength() << " degrees" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCorridor(params);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Found " << stars.size() << " stars in " << duration.count() << " ms" << std::endl;
    }
    
    // Test 3: JSON corridor query - including target star
    std::cout << "\n=== Test 3: JSON corridor query ===" << std::endl;
    {
        // Create corridor that includes star 3411546266140512128 at RA=73.4161, Dec=20.3317
        std::string json = R"({
            "path": [
                {"ra": 73.0, "dec": 20.0},
                {"ra": 74.0, "dec": 21.0}
            ],
            "width": 0.5,
            "max_magnitude": 15.0,
            "min_parallax": 0.0,
            "max_results": 2000
        })";
        
        std::cout << "JSON query:\n" << json << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCorridorJSON(json);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Found " << stars.size() << " stars in " << duration.count() << " ms" << std::endl;
        
        // Check if target star is in results
        bool found_target = false;
        for (const auto& star : stars) {
            if (star.source_id == 3411546266140512128ULL) {
                found_target = true;
                std::cout << "✓ Found target star 3411546266140512128!" << std::endl;
                std::cout << "  RA=" << star.ra << " Dec=" << star.dec 
                          << " G=" << star.phot_g_mean_mag << std::endl;
                break;
            }
        }
        if (!found_target) {
            std::cout << "✗ Target star 3411546266140512128 NOT found in corridor" << std::endl;
        }
    }
    
    // Test 4: Serialize CorridorQueryParams to JSON
    std::cout << "\n=== Test 4: JSON serialization ===" << std::endl;
    {
        ioc::gaia::CorridorQueryParams params;
        params.path = {
            {100.0, 30.0},
            {105.0, 32.0},
            {110.0, 30.0}
        };
        params.width = 0.3;
        params.max_magnitude = 12.0;
        params.min_parallax = 0.5;
        params.max_results = 100;
        
        std::string json = params.toJSON();
        std::cout << "Serialized JSON:\n" << json << std::endl;
        
        // Parse it back
        auto parsed = ioc::gaia::CorridorQueryParams::fromJSON(json);
        std::cout << "\nParsed back:" << std::endl;
        std::cout << "  Path points: " << parsed.path.size() << std::endl;
        std::cout << "  Width: " << parsed.width << std::endl;
        std::cout << "  Max magnitude: " << parsed.max_magnitude << std::endl;
        std::cout << "  Min parallax: " << parsed.min_parallax << std::endl;
        std::cout << "  Max results: " << parsed.max_results << std::endl;
    }
    
    std::cout << "\n=== All tests completed ===" << std::endl;
    
    return 0;
}
