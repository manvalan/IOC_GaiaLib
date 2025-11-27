#include "ioc_gaialib/gaia_catalog.h"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace ioc_gaialib;
using namespace ioc::gaia;

void printSeparator() {
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
}

void printStar(const GaiaStar& star) {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  source_id: " << star.source_id << "\n";
    std::cout << "  RA:  " << std::setw(12) << star.ra << "°\n";
    std::cout << "  Dec: " << std::setw(12) << star.dec << "°\n";
    std::cout << std::setprecision(2);
    std::cout << "  G:   " << std::setw(6) << star.phot_g_mean_mag << " mag\n";
    std::cout << "  BP:  " << std::setw(6) << star.phot_bp_mean_mag << " mag\n";
    std::cout << "  RP:  " << std::setw(6) << star.phot_rp_mean_mag << " mag\n";
}

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  GaiaCatalog - Unified Gaia API Test Suite                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mag18_catalog.gz> [grappa_path]\n\n";
        std::cerr << "Examples:\n";
        std::cerr << "  " << argv[0] << " /path/to/gaia_mag18.cat.gz\n";
        std::cerr << "  " << argv[0] << " /path/to/gaia_mag18.cat.gz /path/to/GRAPPA3E\n\n";
        return 1;
    }
    
    // ====================================================================
    // SETUP: Configure GaiaCatalog
    // ====================================================================
    std::cout << "📂 SETUP: Configuring GaiaCatalog\n";
    printSeparator();
    
    GaiaCatalogConfig config;
    config.mag18_catalog_path = argv[1];
    
    if (argc >= 3) {
        config.grappa_catalog_path = argv[2];
    }
    
    config.enable_caching = true;
    config.cache_size = 10000;
    config.prefer_mag18_for_source_id = true;
    config.prefer_grappa_for_cone = true;
    config.cone_size_threshold = 5.0;
    config.enable_online_fallback = false;
    
    std::cout << "Configuration:\n";
    std::cout << "  Mag18:  " << config.mag18_catalog_path << "\n";
    std::cout << "  GRAPPA: " << (config.grappa_catalog_path.empty() ? "(not configured)" : config.grappa_catalog_path) << "\n";
    std::cout << "  Cache:  " << (config.enable_caching ? "enabled" : "disabled") << " (size: " << config.cache_size << ")\n";
    std::cout << "  Online: " << (config.enable_online_fallback ? "enabled" : "disabled") << "\n\n";
    
    GaiaCatalog catalog(config);
    catalog.setVerbose(true);
    
    if (!catalog.isReady()) {
        std::cerr << "❌ Catalog not ready - no data sources available\n";
        return 1;
    }
    
    auto stats = catalog.getStatistics();
    std::cout << "✅ Catalog ready!\n";
    std::cout << "  Mag18 loaded:  " << (stats.mag18_loaded ? "✅" : "❌") 
              << " (" << stats.mag18_stars_available << " stars)\n";
    std::cout << "  GRAPPA loaded: " << (stats.grappa_loaded ? "✅" : "❌") 
              << " (" << stats.grappa_stars_available << " stars)\n";
    std::cout << "  Online:        " << (stats.online_enabled ? "✅" : "❌") << "\n\n";
    
    // ====================================================================
    // TEST 1: Query single star by source_id
    // ====================================================================
    std::cout << "TEST 1: Query star by source_id\n";
    printSeparator();
    
    // Use a source_id that should exist
    uint64_t test_source_id = 100000000000;  // Adjust based on your data
    
    std::cout << "Querying source_id: " << test_source_id << "\n\n";
    
    auto star1 = catalog.queryStar(test_source_id);
    if (star1) {
        std::cout << "✅ Star found:\n";
        printStar(*star1);
        
        auto metrics = catalog.getLastQueryMetrics();
        std::cout << "\n  Source: " << metrics.source 
                  << ", Duration: " << std::fixed << std::setprecision(3) 
                  << metrics.duration_ms << " ms\n";
    } else {
        std::cout << "⚠️  Star not found (try a different source_id)\n";
    }
    std::cout << "\n";
    
    // ====================================================================
    // TEST 2: Query same star again (cache test)
    // ====================================================================
    std::cout << "TEST 2: Query same star again (cache test)\n";
    printSeparator();
    
    auto star2 = catalog.queryStar(test_source_id);
    if (star2) {
        auto metrics = catalog.getLastQueryMetrics();
        std::cout << "✅ Star retrieved from: " << metrics.source 
                  << " in " << std::fixed << std::setprecision(3) 
                  << metrics.duration_ms << " ms\n";
        
        if (metrics.source == "cache") {
            std::cout << "   🎯 Cache hit! Much faster than first query.\n";
        }
    }
    std::cout << "\n";
    
    // ====================================================================
    // TEST 3: Cone search (small radius)
    // ====================================================================
    std::cout << "TEST 3: Cone search (small radius = 0.5°)\n";
    printSeparator();
    
    double ra = 180.0, dec = 0.0, radius = 0.5;
    std::cout << "Position: RA=" << ra << "°, Dec=" << dec << "°, radius=" << radius << "°\n";
    std::cout << "Max results: 10\n\n";
    
    auto cone_small = catalog.queryCone(ra, dec, radius, 10);
    
    std::cout << "✅ Found " << cone_small.size() << " stars\n";
    
    auto metrics3 = catalog.getLastQueryMetrics();
    std::cout << "  Source: " << metrics3.source 
              << ", Duration: " << std::fixed << std::setprecision(1) 
              << metrics3.duration_ms << " ms\n";
    
    if (!cone_small.empty()) {
        std::cout << "\nFirst 3 stars:\n";
        for (size_t i = 0; i < std::min(size_t(3), cone_small.size()); ++i) {
            std::cout << "\n" << (i+1) << ".\n";
            printStar(cone_small[i]);
        }
    }
    std::cout << "\n";
    
    // ====================================================================
    // TEST 4: Cone search (large radius)
    // ====================================================================
    std::cout << "TEST 4: Cone search (large radius = 10°)\n";
    printSeparator();
    
    radius = 10.0;
    std::cout << "Position: RA=" << ra << "°, Dec=" << dec << "°, radius=" << radius << "°\n";
    std::cout << "Max results: 100\n\n";
    
    auto cone_large = catalog.queryCone(ra, dec, radius, 100);
    
    std::cout << "✅ Found " << cone_large.size() << " stars\n";
    
    auto metrics4 = catalog.getLastQueryMetrics();
    std::cout << "  Source: " << metrics4.source 
              << ", Duration: " << std::fixed << std::setprecision(1) 
              << metrics4.duration_ms << " ms\n";
    
    std::cout << "  Note: Large radius should use GRAPPA3E if available (faster)\n\n";
    
    // ====================================================================
    // TEST 5: Magnitude filter
    // ====================================================================
    std::cout << "TEST 5: Cone search with magnitude filter\n";
    printSeparator();
    
    double mag_min = 12.0, mag_max = 14.0;
    radius = 2.0;
    
    std::cout << "Position: RA=" << ra << "°, Dec=" << dec << "°, radius=" << radius << "°\n";
    std::cout << "Magnitude: " << mag_min << " ≤ G ≤ " << mag_max << "\n";
    std::cout << "Max results: 10\n\n";
    
    auto mag_filtered = catalog.queryConeWithMagnitude(ra, dec, radius, mag_min, mag_max, 10);
    
    std::cout << "✅ Found " << mag_filtered.size() << " stars\n";
    
    auto metrics5 = catalog.getLastQueryMetrics();
    std::cout << "  Source: " << metrics5.source 
              << ", Duration: " << std::fixed << std::setprecision(1) 
              << metrics5.duration_ms << " ms\n";
    
    if (!mag_filtered.empty()) {
        std::cout << "\nBright stars found:\n";
        for (const auto& s : mag_filtered) {
            std::cout << "  G=" << std::setprecision(2) << s.phot_g_mean_mag 
                      << " (source_id=" << s.source_id << ")\n";
        }
    }
    std::cout << "\n";
    
    // ====================================================================
    // TEST 6: Find brightest stars
    // ====================================================================
    std::cout << "TEST 6: Find 5 brightest stars in region\n";
    printSeparator();
    
    radius = 5.0;
    std::cout << "Position: RA=" << ra << "°, Dec=" << dec << "°, radius=" << radius << "°\n\n";
    
    auto brightest = catalog.queryBrightest(ra, dec, radius, 5);
    
    std::cout << "✅ 5 brightest stars:\n";
    for (size_t i = 0; i < brightest.size(); ++i) {
        std::cout << "  " << (i+1) << ". G=" << std::setprecision(2) 
                  << brightest[i].phot_g_mean_mag 
                  << " (source_id=" << brightest[i].source_id << ")\n";
    }
    
    auto metrics6 = catalog.getLastQueryMetrics();
    std::cout << "\n  Source: " << metrics6.source 
              << ", Duration: " << std::fixed << std::setprecision(1) 
              << metrics6.duration_ms << " ms\n\n";
    
    // ====================================================================
    // TEST 7: Count stars
    // ====================================================================
    std::cout << "TEST 7: Count stars in region\n";
    printSeparator();
    
    radius = 3.0;
    std::cout << "Position: RA=" << ra << "°, Dec=" << dec << "°, radius=" << radius << "°\n\n";
    
    size_t count = catalog.countInCone(ra, dec, radius);
    
    std::cout << "✅ Stars in region: " << count << "\n";
    
    auto metrics7 = catalog.getLastQueryMetrics();
    std::cout << "  Source: " << metrics7.source 
              << ", Duration: " << std::fixed << std::setprecision(1) 
              << metrics7.duration_ms << " ms\n\n";
    
    // ====================================================================
    // TEST 8: Batch query
    // ====================================================================
    std::cout << "TEST 8: Batch query (multiple source_ids)\n";
    printSeparator();
    
    std::vector<uint64_t> source_ids = {
        test_source_id,
        test_source_id + 1000,
        test_source_id + 2000,
        test_source_id + 3000,
        test_source_id + 4000
    };
    
    std::cout << "Querying " << source_ids.size() << " source_ids...\n\n";
    
    auto batch_results = catalog.queryStars(source_ids);
    
    std::cout << "✅ Found " << batch_results.size() << "/" << source_ids.size() << " stars\n\n";
    
    // ====================================================================
    // STATISTICS
    // ====================================================================
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  FINAL STATISTICS                                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    auto final_stats = catalog.getStatistics();
    auto [cache_hits, cache_misses] = catalog.getCacheStats();
    
    std::cout << "Query Statistics:\n";
    std::cout << "  Total queries:   " << final_stats.total_queries << "\n";
    std::cout << "  Mag18 queries:   " << final_stats.mag18_queries << "\n";
    std::cout << "  GRAPPA queries:  " << final_stats.grappa_queries << "\n";
    std::cout << "  Online queries:  " << final_stats.online_queries << "\n";
    std::cout << "\nCache Statistics:\n";
    std::cout << "  Cache hits:      " << cache_hits << "\n";
    std::cout << "  Cache misses:    " << cache_misses << "\n";
    
    if (cache_hits + cache_misses > 0) {
        double hit_rate = 100.0 * cache_hits / (cache_hits + cache_misses);
        std::cout << "  Hit rate:        " << std::fixed << std::setprecision(1) 
                  << hit_rate << "%\n";
    }
    
    std::cout << "\n✅ GaiaCatalog is the recommended API for all Gaia queries!\n\n";
    
    return 0;
}
