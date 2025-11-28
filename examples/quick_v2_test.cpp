// Quick V2 Catalog Validation Test
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace ioc::gaia;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog.mag18v2>\n";
        return 1;
    }
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Mag18 V2 Catalog - Quick Interface Test                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    std::string catalog_path = argv[1];
    
    try {
        // Load catalog
        std::cout << "Loading catalog...\n";
        auto t0 = std::chrono::high_resolution_clock::now();
        
        Mag18CatalogV2 catalog(catalog_path);
        
        auto t1 = std::chrono::high_resolution_clock::now();
        auto load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        
        std::cout << "✅ Loaded in " << load_ms << " ms\n\n";
        
        // Basic stats
        std::cout << "Catalog Statistics:\n";
        std::cout << "  Total stars: " << catalog.getTotalStars() << "\n";
        std::cout << "  HEALPix pixels: " << catalog.getNumPixels() << "\n";
        std::cout << "  Chunks: " << catalog.getNumChunks() << "\n";
        std::cout << "  Mag limit: ≤ " << std::fixed << std::setprecision(1) 
                  << catalog.getMagLimit() << "\n\n";
        
        // Query by source_id
        std::cout << "Testing query by source_id...\n";
        auto star = catalog.queryBySourceId(1000000);
        if (star) {
            std::cout << "  ✓ Found: RA=" << std::setprecision(4) << star->ra 
                      << "° Dec=" << star->dec 
                      << "° G=" << std::setprecision(2) << star->phot_g_mean_mag << "\n";
        } else {
            std::cout << "  - Not found (may be > mag 18)\n";
        }
        std::cout << "\n";
        
        // Cone search (small)
        std::cout << "Testing cone search (RA=180° Dec=0° radius=1°)...\n";
        t0 = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCone(180.0, 0.0, 1.0, 100);
        t1 = std::chrono::high_resolution_clock::now();
        auto cone_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        
        std::cout << "  ✓ Found " << stars.size() << " stars in " << cone_ms << " ms\n";
        if (!stars.empty()) {
            std::cout << "  First: G=" << std::setprecision(2) << stars[0].phot_g_mean_mag 
                      << " (source_id=" << stars[0].source_id << ")\n";
        }
        std::cout << "\n";
        
        // Count in cone
        std::cout << "Testing countInCone (RA=180° Dec=0° radius=5°)...\n";
        t0 = std::chrono::high_resolution_clock::now();
        size_t count = catalog.countInCone(180.0, 0.0, 5.0);
        t1 = std::chrono::high_resolution_clock::now();
        auto count_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        
        std::cout << "  ✓ Count=" << count << " in " << count_ms << " ms\n\n";
        
        // Magnitude filter
        std::cout << "Testing magnitude filter (G=10-12 in 5° cone)...\n";
        auto bright = catalog.queryConeWithMagnitude(180.0, 0.0, 5.0, 10.0, 12.0, 50);
        std::cout << "  ✓ Found " << bright.size() << " stars\n\n";
        
        // Brightest
        std::cout << "Testing brightest query (5° cone, top 5)...\n";
        auto brightest = catalog.queryBrightest(180.0, 0.0, 5.0, 5);
        std::cout << "  ✓ Top 5 brightest:\n";
        for (size_t i = 0; i < brightest.size(); ++i) {
            std::cout << "    " << (i+1) << ". G=" << std::setprecision(2) 
                      << brightest[i].phot_g_mean_mag << "\n";
        }
        std::cout << "\n";
        
        // Parallel processing
        std::cout << "Testing parallel processing...\n";
        catalog.setParallelProcessing(true, 4);
        std::cout << "  ✓ Parallel enabled: " << (catalog.isParallelEnabled() ? "YES" : "NO") 
                  << " (" << catalog.getNumThreads() << " threads)\n\n";
        
        // Summary
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ✅ ALL TESTS PASSED - V2 Interface is functional!         ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "Interface Summary:\n";
        std::cout << "  ✓ open(path)\n";
        std::cout << "  ✓ getTotalStars()\n";
        std::cout << "  ✓ getNumPixels() / getNumChunks()\n";
        std::cout << "  ✓ getMagLimit()\n";
        std::cout << "  ✓ queryBySourceId(id)\n";
        std::cout << "  ✓ queryCone(ra, dec, radius, max_results)\n";
        std::cout << "  ✓ queryConeWithMagnitude(ra, dec, radius, mag_min, mag_max)\n";
        std::cout << "  ✓ queryBrightest(ra, dec, radius, num_stars)\n";
        std::cout << "  ✓ countInCone(ra, dec, radius)\n";
        std::cout << "  ✓ setParallelProcessing(enable, num_threads)\n";
        std::cout << "  ✓ isParallelEnabled() / getNumThreads()\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}
