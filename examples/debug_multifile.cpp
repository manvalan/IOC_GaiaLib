#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <algorithm>
#include "../include/ioc_gaialib/multifile_catalog_v2.h"

using namespace ioc::gaia;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <multifile_catalog_directory>" << std::endl;
        return 1;
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ Multi-File V2 Catalog Debug Test                         ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝\n" << std::endl;

    try {
        // Load catalog
        std::cout << "Loading catalog from: " << argv[1] << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        
        MultiFileCatalogV2 catalog(argv[1]);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "✅ Catalog loaded in " << duration.count() << " ms\n" << std::endl;

        // Print basic info
        std::cout << "📊 Basic Information:" << std::endl;
        std::cout << "   Total stars: " << catalog.getTotalStars() << std::endl;
        std::cout << "   Total chunks: " << catalog.getNumChunks() << std::endl;
        std::cout << "   Magnitude limit: " << catalog.getMagLimit() << std::endl;

        // Test HEALPix calculations
        std::cout << "\n🧭 Testing HEALPix calculations:" << std::endl;
        
        // Test some known coordinates
        std::vector<std::pair<double, double>> test_coords = {
            {0.0, 0.0},        // Equator
            {83.63, 22.01},    // Pleiades
            {266.4, -29.0},    // Galactic center
            {0.0, 90.0},       // North pole
            {180.0, 0.0}       // Opposite side
        };
        
        for (const auto& coord : test_coords) {
            uint32_t pixel = catalog.getHEALPixPixel(coord.first, coord.second);
            std::cout << "   (" << std::fixed << std::setprecision(2) 
                      << coord.first << "°, " << coord.second << "°) -> pixel " << pixel << std::endl;
        }

        // Test pixel finding for cone searches
        std::cout << "\n🔍 Testing cone search:" << std::endl;
        
        // Test small cone around Pleiades
        double ra = 83.63, dec = 22.01, radius = 1.0;
        std::cout << "   Searching cone: RA=" << ra << "°, Dec=" << dec 
                  << "°, radius=" << radius << "°" << std::endl;
        
        uint32_t test_pixel = catalog.getHEALPixPixel(ra, dec);
        std::cout << "   Center pixel: " << test_pixel << std::endl;

        // Let's also test a few larger radius searches
        std::vector<double> test_radii = {0.1, 0.5, 1.0, 2.0, 5.0};
        for (double test_radius : test_radii) {
            auto test_stars = catalog.queryCone(ra, dec, test_radius);
            std::cout << "   Radius " << test_radius << "°: " << test_stars.size() << " stars" << std::endl;
        }

        // Test different locations
        std::vector<std::tuple<double, double, std::string>> locations = {
            {0.0, 0.0, "Equator"},
            {83.63, 22.01, "Pleiades"},
            {266.4, -29.0, "Galactic Center"},
            {56.75, 24.12, "Alternative Pleiades"},
            {180.0, 0.0, "Opposite side"}
        };

        std::cout << "\n🌍 Testing different sky regions:" << std::endl;
        for (const auto& [test_ra, test_dec, name] : locations) {
            auto test_stars = catalog.queryCone(test_ra, test_dec, 1.0);
            uint32_t pixel = catalog.getHEALPixPixel(test_ra, test_dec);
            std::cout << "   " << name << " (" << test_ra << "°, " << test_dec << "°) -> pixel " 
                      << pixel << " -> " << test_stars.size() << " stars" << std::endl;
        }

        // Test actual cone search
        std::cout << "\n🎯 Testing actual cone search:" << std::endl;
        auto search_start = std::chrono::high_resolution_clock::now();
        
        auto stars = catalog.queryCone(ra, dec, radius);
        
        auto search_end = std::chrono::high_resolution_clock::now();
        auto search_duration = std::chrono::duration_cast<std::chrono::microseconds>(search_end - search_start);
        
        std::cout << "   Query: RA=" << ra << "°, Dec=" << dec << "°, radius=" << radius << "°" << std::endl;
        std::cout << "   Found " << stars.size() << " stars in " << search_duration.count() 
                  << " μs (" << (search_duration.count() / 1000.0) << " ms)" << std::endl;
        
        if (!stars.empty()) {
            std::cout << "   First 5 stars found:" << std::endl;
            for (size_t i = 0; i < std::min<size_t>(5, stars.size()); i++) {
                const auto& star = stars[i];
                std::cout << "     RA=" << std::fixed << std::setprecision(4) << star.ra 
                          << "°, Dec=" << star.dec << "°, Mag=" << star.phot_g_mean_mag << std::endl;
            }
        }

        std::cout << "\n📈 Cache Statistics:" << std::endl;
        std::cout << "   Cache functionality is internal" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}