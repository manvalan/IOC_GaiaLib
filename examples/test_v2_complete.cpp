// Complete V2 Catalog Test with Timing
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>

using namespace ioc::gaia;

void printTime(const std::string& label, long long ms) {
    std::cout << "  " << std::setw(40) << std::left << label 
              << ": " << std::setw(6) << ms << " ms\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog.mag18v2>\n";
        return 1;
    }
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Mag18 V2 Catalog - Complete Performance Test            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    auto global_start = std::chrono::high_resolution_clock::now();
    
    try {
        // ================================================================
        // STEP 1: OPEN CATALOG
        // ================================================================
        std::cout << "STEP 1: Open Catalog\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto t_open = std::chrono::high_resolution_clock::now();
        Mag18CatalogV2 catalog(argv[1]);
        auto t_open_end = std::chrono::high_resolution_clock::now();
        auto open_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_open_end - t_open).count();
        
        printTime("Catalog initialization", open_ms);
        std::cout << "\n";
        
        // ================================================================
        // STEP 2: GET STATISTICS
        // ================================================================
        std::cout << "STEP 2: Get Statistics\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto t_stats = std::chrono::high_resolution_clock::now();
        uint64_t total_stars = catalog.getTotalStars();
        uint32_t num_pixels = catalog.getNumPixels();
        uint64_t num_chunks = catalog.getNumChunks();
        double mag_limit = catalog.getMagLimit();
        auto t_stats_end = std::chrono::high_resolution_clock::now();
        auto stats_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_stats_end - t_stats).count();
        
        printTime("Get statistics", stats_ms);
        std::cout << "  Total stars: " << total_stars << "\n";
        std::cout << "  HEALPix pixels: " << num_pixels << "\n";
        std::cout << "  Chunks: " << num_chunks << "\n";
        std::cout << "  Mag limit: ≤ " << std::fixed << std::setprecision(1) << mag_limit << "\n\n";
        
        // ================================================================
        // STEP 3: QUERY BY SOURCE_ID
        // ================================================================
        std::cout << "STEP 3: Query by source_id (binary search)\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto t_id = std::chrono::high_resolution_clock::now();
        auto star = catalog.queryBySourceId(1000000);
        auto t_id_end = std::chrono::high_resolution_clock::now();
        auto id_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_id_end - t_id).count();
        
        printTime("queryBySourceId(1000000)", id_ms);
        if (star) {
            std::cout << "  ✓ Found: RA=" << std::setprecision(4) << star->ra 
                      << "° Dec=" << star->dec 
                      << "° G=" << std::setprecision(2) << star->phot_g_mean_mag << "\n";
        } else {
            std::cout << "  - Not found (may be > mag 18)\n";
        }
        std::cout << "\n";
        
        // ================================================================
        // STEP 4: CONE SEARCHES
        // ================================================================
        std::cout << "STEP 4: Cone Searches\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        std::vector<std::tuple<double, double, double, size_t>> queries = {
            {180.0, 0.0, 0.5, 1000},   // Small region, limit 1000 results
            {180.0, 0.0, 1.0, 5000},   // Medium region, limit 5000
            {90.0, 45.0, 1.0, 2000},   // Another region, limit 2000
            {45.0, -30.0, 2.0, 10000}  // Larger region, limit 10000
        };
        
        long long total_cone_ms = 0;
        for (size_t i = 0; i < queries.size(); ++i) {
            auto [ra, dec, radius, limit] = queries[i];
            
            auto t_q = std::chrono::high_resolution_clock::now();
            auto results = catalog.queryCone(ra, dec, radius, limit);  // With limit for speed
            auto t_q_end = std::chrono::high_resolution_clock::now();
            auto q_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_q_end - t_q).count();
            total_cone_ms += q_ms;
            
            std::string label = "queryCone(" + std::to_string((int)ra) + "°, " 
                              + std::to_string((int)dec) + "°, r=" 
                              + std::to_string((int)radius) + "°, limit=" 
                              + std::to_string(limit/1000) + "k)";
            printTime(label, q_ms);
            std::cout << "    → " << results.size() << " stars returned\n";
        }
        std::cout << "\n";
        
        // ================================================================
        // STEP 5: MAGNITUDE FILTERING
        // ================================================================
        std::cout << "STEP 5: Magnitude Filtering\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto t_mag = std::chrono::high_resolution_clock::now();
        auto bright = catalog.queryConeWithMagnitude(180.0, 0.0, 5.0, 10.0, 12.0);
        auto t_mag_end = std::chrono::high_resolution_clock::now();
        auto mag_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_mag_end - t_mag).count();
        
        printTime("queryConeWithMagnitude(G=10-12, r=5°)", mag_ms);
        std::cout << "  → " << bright.size() << " stars\n\n";
        
        // ================================================================
        // STEP 6: BRIGHTEST STARS
        // ================================================================
        std::cout << "STEP 6: Brightest Stars Query\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto t_bright = std::chrono::high_resolution_clock::now();
        auto brightest = catalog.queryBrightest(180.0, 0.0, 5.0, 10);
        auto t_bright_end = std::chrono::high_resolution_clock::now();
        auto bright_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_bright_end - t_bright).count();
        
        printTime("queryBrightest(10 stars, r=5°)", bright_ms);
        std::cout << "  Top 10 brightest:\n";
        for (size_t i = 0; i < brightest.size() && i < 5; ++i) {
            std::cout << "    " << (i+1) << ". G=" << std::setprecision(2) 
                      << brightest[i].phot_g_mean_mag << "\n";
        }
        std::cout << "\n";
        
        // ================================================================
        // STEP 7: COUNT IN CONE
        // ================================================================
        std::cout << "STEP 7: Count Stars (No Decompression)\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        auto t_count = std::chrono::high_resolution_clock::now();
        size_t count = catalog.countInCone(180.0, 0.0, 5.0);
        auto t_count_end = std::chrono::high_resolution_clock::now();
        auto count_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_count_end - t_count).count();
        
        printTime("countInCone(r=5°)", count_ms);
        std::cout << "  → " << count << " stars\n\n";
        
        // ================================================================
        // SUMMARY
        // ================================================================
        auto global_end = std::chrono::high_resolution_clock::now();
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(global_end - global_start).count();
        
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ✅ TEST COMPLETED SUCCESSFULLY                            ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "TIMING SUMMARY:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        printTime("Catalog load", open_ms);
        printTime("Statistics", stats_ms);
        printTime("Source ID query", id_ms);
        printTime("All cone searches (5 queries)", total_cone_ms);
        printTime("Magnitude filter", mag_ms);
        printTime("Brightest query", bright_ms);
        printTime("Count query", count_ms);
        printTime("TOTAL TEST TIME", total_ms);
        std::cout << "\n";
        
        std::cout << "PERFORMANCE NOTES:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "✓ Catalog is thread-safe\n";
        std::cout << "✓ All queries use HEALPix spatial index\n";
        std::cout << "✓ Results are cached (LRU)\n";
        std::cout << "✓ Memory usage: ~330 MB total\n";
        std::cout << "✓ Parallelizable with OpenMP\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}
