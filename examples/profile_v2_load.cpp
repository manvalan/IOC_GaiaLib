// Profile V2 Catalog Initialization Time
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace ioc::gaia;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog.mag18v2>\n";
        return 1;
    }
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  V2 Catalog Load Time Profiling                            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    auto t_start = std::chrono::high_resolution_clock::now();
    auto t_last = t_start;
    
    auto print_step = [&](const std::string& step) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed_total = std::chrono::duration_cast<std::chrono::milliseconds>(now - t_start).count();
        auto elapsed_step = std::chrono::duration_cast<std::chrono::milliseconds>(now - t_last).count();
        std::cout << "[" << std::setw(6) << elapsed_total << "ms total | "
                  << std::setw(6) << elapsed_step << "ms this step] " << step << "\n";
        t_last = now;
    };
    
    print_step("Starting...");
    
    try {
        print_step("About to construct Mag18CatalogV2...");
        
        Mag18CatalogV2 catalog(argv[1]);
        
        print_step("✅ Constructor completed");
        
        // Try first query
        print_step("About to call getTotalStars()...");
        auto total = catalog.getTotalStars();
        print_step("✅ getTotalStars() = " + std::to_string(total));
        
        print_step("About to call getNumPixels()...");
        auto pixels = catalog.getNumPixels();
        print_step("✅ getNumPixels() = " + std::to_string(pixels));
        
        print_step("About to call queryCone (small 1°)...");
        auto stars = catalog.queryCone(180.0, 0.0, 1.0, 100);
        print_step("✅ queryCone() returned " + std::to_string(stars.size()) + " stars");
        
        print_step("About to call queryBySourceId()...");
        auto star = catalog.queryBySourceId(1000000);
        print_step("✅ queryBySourceId() done");
        
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - t_start).count();
        
        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  Total Time: " << std::setw(47) << (std::to_string(total_ms) + "ms") << "║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        print_step("❌ Exception: " + std::string(e.what()));
        return 1;
    }
}
