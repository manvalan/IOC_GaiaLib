#include <iostream>
#include <chrono>
#include "../include/ioc_gaialib/multifile_catalog_v2.h"

using namespace ioc::gaia;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <multifile_catalog_directory>" << std::endl;
        return 1;
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ Targeted Cone Search Test                                 ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝\n" << std::endl;

    try {
        MultiFileCatalogV2 catalog(argv[1]);
        std::cout << "✅ Catalog loaded" << std::endl;

        // Test coordinates from the actual data we know exists
        std::vector<std::tuple<double, double, std::string>> test_coords = {
            {0.0763236, 0.0557768, "First star from chunk"},
            {0.11395, 0.0402646, "Second star from chunk"},
            {0.1, 0.1, "Near first stars"},
            {0.0, 0.0, "Exact zero"},
            {1.0, 1.0, "Near first region"},
            {2.0, 2.0, "Slightly further"}
        };

        for (const auto& [ra, dec, description] : test_coords) {
            std::cout << "\n🎯 Testing " << description << " (" << ra << "°, " << dec << "°):" << std::endl;
            
            uint32_t pixel = catalog.getHEALPixPixel(ra, dec);
            std::cout << "   Calculated pixel: " << pixel << std::endl;
            
            auto start = std::chrono::high_resolution_clock::now();
            auto stars = catalog.queryCone(ra, dec, 0.5);  // 0.5° radius
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            std::cout << "   Found " << stars.size() << " stars in " << duration.count() << " μs" << std::endl;
            
            if (!stars.empty()) {
                std::cout << "   ✅ SUCCESS! First star: RA=" << stars[0].ra 
                          << "°, Dec=" << stars[0].dec << "°, Mag=" << stars[0].phot_g_mean_mag << std::endl;
                return 0;  // Found stars, mission accomplished
            }
        }

        std::cout << "\n❌ No stars found in any test region" << std::endl;
        
        // Let's try to understand if there's a pixel numbering issue
        std::cout << "\n🔍 Testing pixel 1 directly (should contain first stars):" << std::endl;
        
        // We know pixel 1 contains the first 916 stars, but let's see which coordinates give pixel 1
        bool found_pixel_1 = false;
        for (double test_ra = 0.0; test_ra <= 3.0 && !found_pixel_1; test_ra += 0.1) {
            for (double test_dec = 0.0; test_dec <= 2.0 && !found_pixel_1; test_dec += 0.1) {
                uint32_t test_pixel = catalog.getHEALPixPixel(test_ra, test_dec);
                if (test_pixel == 1) {
                    std::cout << "   Found coordinates that give pixel 1: (" 
                              << test_ra << "°, " << test_dec << "°)" << std::endl;
                    
                    auto test_stars = catalog.queryCone(test_ra, test_dec, 0.5);
                    std::cout << "   Stars found: " << test_stars.size() << std::endl;
                    found_pixel_1 = true;
                }
            }
        }
        
        if (!found_pixel_1) {
            std::cout << "   ❌ Could not find coordinates that map to pixel 1" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}