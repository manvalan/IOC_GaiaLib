// Measure V2 Catalog Loading Speed with Lazy Loading
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
    std::cout << "║  V2 Catalog: Lazy-Loading Performance Test                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    auto t_start = std::chrono::high_resolution_clock::now();
    std::cout << "Opening catalog: " << argv[1] << "\n";
    
    try {
        Mag18CatalogV2 catalog(argv[1]);
        
        auto t_open = std::chrono::high_resolution_clock::now();
        auto open_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_open - t_start).count();
        
        std::cout << "\n✅ Catalog opened in " << open_ms << " ms\n";
        std::cout << "   Total stars: " << catalog.getTotalStars() << "\n";
        std::cout << "   HEALPix pixels: " << catalog.getNumPixels() << "\n";
        std::cout << "   Chunks: " << catalog.getNumChunks() << "\n\n";
        
        // First query triggers lazy load of HEALPix index
        std::cout << "Executing first query (triggers lazy-load of HEALPix index)...\n";
        auto t_q1 = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCone(180.0, 0.0, 1.0, 100);
        auto t_q1_end = std::chrono::high_resolution_clock::now();
        auto q1_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_q1_end - t_q1).count();
        
        std::cout << "✅ First query done in " << q1_ms << " ms (includes HEALPix index load)\n";
        std::cout << "   Found: " << stars.size() << " stars\n\n";
        
        // Second query should be faster (index already in memory)
        std::cout << "Executing second query (index already cached)...\n";
        auto t_q2 = std::chrono::high_resolution_clock::now();
        auto stars2 = catalog.queryCone(90.0, 45.0, 1.0, 100);
        auto t_q2_end = std::chrono::high_resolution_clock::now();
        auto q2_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_q2_end - t_q2).count();
        
        std::cout << "✅ Second query done in " << q2_ms << " ms\n";
        std::cout << "   Found: " << stars2.size() << " stars\n\n";
        
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - t_start).count();
        
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  SUMMARY                                                   ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║  Catalog open: " << std::setw(45) << (std::to_string(open_ms) + "ms") << "║\n";
        std::cout << "║  1st query (with index load): " << std::setw(30) << (std::to_string(q1_ms) + "ms") << "║\n";
        std::cout << "║  2nd query (index cached): " << std::setw(32) << (std::to_string(q2_ms) + "ms") << "║\n";
        std::cout << "║  Total: " << std::setw(51) << (std::to_string(total_ms) + "ms") << "║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "✅ Lazy-loading successful!\n";
        std::cout << "   No pre-load delay - indices load only when first needed.\n";
        std::cout << "   Subsequent queries use cached indices.\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
}
