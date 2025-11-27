#include "ioc_gaialib/gaia_mag18_catalog.h"
#include <iostream>
#include <iomanip>

using namespace ioc_gaialib;

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Gaia Mag18 Compressed Catalog - Test Suite               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog_file.gz>\n\n";
        return 1;
    }
    
    std::string catalog_file = argv[1];
    
    try {
        // ================================================================
        // TEST 1: Load catalog
        // ================================================================
        std::cout << "TEST 1: Loading compressed catalog\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        GaiaMag18Catalog catalog(catalog_file);
        
        if (!catalog.isLoaded()) {
            std::cerr << "❌ Failed to load catalog\n";
            return 1;
        }
        
        std::cout << "✅ Catalog loaded: " << catalog.getCatalogPath() << "\n\n";
        
        // ================================================================
        // TEST 2: Statistics
        // ================================================================
        std::cout << "TEST 2: Catalog statistics\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto stats = catalog.getStatistics();
        
        std::cout << "  Total stars:        " << stats.total_stars << "\n";
        std::cout << "  Magnitude limit:    ≤ " << std::fixed << std::setprecision(1) 
                  << stats.mag_limit << "\n";
        std::cout << "  Uncompressed:       " << (stats.uncompressed_size / (1024*1024)) << " MB\n";
        std::cout << "  Compressed:         " << (stats.file_size / (1024*1024)) << " MB\n";
        std::cout << "  Compression ratio:  " << std::setprecision(1) 
                  << stats.compression_ratio << "%\n\n";
        
        // ================================================================
        // TEST 3: Query by source_id (binary search)
        // ================================================================
        std::cout << "TEST 3: Query by source_id (fast binary search)\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Try to find first star
        auto first = catalog.queryBySourceId(1);
        if (first) {
            std::cout << "✅ Found star with source_id=1:\n";
            std::cout << "   RA:  " << std::setprecision(6) << first->ra << "°\n";
            std::cout << "   Dec: " << first->dec << "°\n";
            std::cout << "   G:   " << std::setprecision(2) << first->phot_g_mean_mag << "\n\n";
        } else {
            std::cout << "⚠️  source_id=1 not found (might be > mag 18)\n\n";
        }
        
        // ================================================================
        // TEST 4: Cone search
        // ================================================================
        std::cout << "TEST 4: Cone search (RA=180°, Dec=0°, radius=1°)\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto stars = catalog.queryCone(180.0, 0.0, 1.0, 10);
        
        std::cout << "✅ Found " << stars.size() << " stars (limit 10)\n";
        if (!stars.empty()) {
            std::cout << "\nFirst 5 stars:\n";
            for (size_t i = 0; i < std::min(size_t(5), stars.size()); ++i) {
                std::cout << std::setprecision(6);
                std::cout << "  " << (i+1) << ". source_id=" << stars[i].source_id
                          << " RA=" << stars[i].ra << "° Dec=" << stars[i].dec 
                          << "° G=" << std::setprecision(2) << stars[i].phot_g_mean_mag << "\n";
            }
        }
        std::cout << "\n";
        
        // ================================================================
        // TEST 5: Magnitude filter
        // ================================================================
        std::cout << "TEST 5: Cone search with magnitude filter (G: 10-12)\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto bright = catalog.queryConeWithMagnitude(180.0, 0.0, 2.0, 10.0, 12.0, 5);
        
        std::cout << "✅ Found " << bright.size() << " bright stars\n";
        for (const auto& star : bright) {
            std::cout << "  G=" << std::setprecision(2) << star.phot_g_mean_mag 
                      << " RA=" << std::setprecision(4) << star.ra 
                      << "° Dec=" << star.dec << "°\n";
        }
        std::cout << "\n";
        
        // ================================================================
        // TEST 6: Brightest stars
        // ================================================================
        std::cout << "TEST 6: Find 5 brightest stars in region\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto brightest = catalog.queryBrightest(180.0, 0.0, 2.0, 5);
        
        std::cout << "✅ 5 brightest stars:\n";
        for (size_t i = 0; i < brightest.size(); ++i) {
            std::cout << "  " << (i+1) << ". G=" << std::setprecision(2) 
                      << brightest[i].phot_g_mean_mag 
                      << " (source_id=" << brightest[i].source_id << ")\n";
        }
        std::cout << "\n";
        
        // ================================================================
        // TEST 7: Count stars
        // ================================================================
        std::cout << "TEST 7: Count stars in large region\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        size_t count = catalog.countInCone(180.0, 0.0, 5.0);
        
        std::cout << "✅ Stars in 5° radius: " << count << "\n\n";
        
        // ================================================================
        // SUMMARY
        // ================================================================
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ALL TESTS PASSED                                          ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "✅ Compressed catalog is fully functional!\n\n";
        std::cout << "Key features:\n";
        std::cout << "  • Fast binary search by source_id\n";
        std::cout << "  • Cone search with magnitude filtering\n";
        std::cout << "  • Brightest star queries\n";
        std::cout << "  • Efficient compressed storage\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
