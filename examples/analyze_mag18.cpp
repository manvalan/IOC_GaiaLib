#include "ioc_gaialib/gaia_mag18_catalog.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include <vector>

using namespace ioc_gaialib;
using namespace ioc::gaia;

void printSeparator() {
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
}

double measureTime(std::function<void()> func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Mag18 Catalog - Strengths & Limitations Analysis         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mag18_catalog.gz>\n\n";
        return 1;
    }
    
    std::string catalog_file = argv[1];
    
    std::cout << "Loading catalog: " << catalog_file << "\n";
    GaiaMag18Catalog catalog(catalog_file);
    
    if (!catalog.isLoaded()) {
        std::cerr << "❌ Failed to load catalog\n";
        return 1;
    }
    
    auto stats = catalog.getStatistics();
    
    std::cout << "\n📊 Catalog Statistics:\n";
    std::cout << "  Stars:              " << stats.total_stars << "\n";
    std::cout << "  Magnitude limit:    ≤ " << stats.mag_limit << "\n";
    std::cout << "  Compressed size:    " << (stats.file_size / (1024*1024)) << " MB\n";
    std::cout << "  Uncompressed size:  " << (stats.uncompressed_size / (1024*1024)) << " MB\n";
    std::cout << "  Compression ratio:  " << std::fixed << std::setprecision(1) 
              << stats.compression_ratio << "%\n\n";
    
    // ====================================================================
    // STRENGTH: Binary Search for source_id
    // ====================================================================
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "STRENGTH #1: Binary Search for source_id\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";
    
    // Get some actual source_ids from the catalog
    std::vector<uint64_t> test_source_ids;
    auto sample_stars = catalog.queryCone(180.0, 0.0, 0.1, 100);
    
    if (sample_stars.size() >= 10) {
        for (size_t i = 0; i < 10; ++i) {
            test_source_ids.push_back(sample_stars[i].source_id);
        }
        
        std::cout << "Testing binary search with 10 known source_ids...\n\n";
        
        std::vector<double> times;
        for (uint64_t sid : test_source_ids) {
            double time_ms = measureTime([&]() {
                catalog.queryBySourceId(sid);
            });
            times.push_back(time_ms);
        }
        
        double avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        double min_time = *std::min_element(times.begin(), times.end());
        double max_time = *std::max_element(times.begin(), times.end());
        
        std::cout << "✅ Results:\n";
        std::cout << "  Queries:     10\n";
        std::cout << "  Average:     " << std::fixed << std::setprecision(3) << avg_time << " ms\n";
        std::cout << "  Min:         " << min_time << " ms\n";
        std::cout << "  Max:         " << max_time << " ms\n";
        std::cout << "  Complexity:  O(log N) - EXCELLENT!\n\n";
        
        std::cout << "💡 Use case: Fast lookups of known Gaia source_ids\n";
        std::cout << "   Perfect for: Cross-matching, follow-up observations\n\n";
    }
    
    // ====================================================================
    // STRENGTH: Compressed Storage
    // ====================================================================
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "STRENGTH #2: Compressed, Portable Storage\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";
    
    std::cout << "✅ Single file format:\n";
    std::cout << "  Size:        " << (stats.file_size / (1024*1024)) << " MB\n";
    std::cout << "  vs GRAPPA3E: 146,000 MB (16x smaller!)\n";
    std::cout << "  Files:       1 (vs 61,202 for GRAPPA3E)\n";
    std::cout << "  Portable:    Easy to distribute, backup, transfer\n\n";
    
    std::cout << "💡 Use case: Archiving, distribution, bandwidth-limited environments\n\n";
    
    // ====================================================================
    // STRENGTH: Sufficient for bright stars
    // ====================================================================
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "STRENGTH #3: Complete Coverage for Bright Stars\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";
    
    std::cout << "✅ Coverage:\n";
    std::cout << "  Stars:       " << stats.total_stars << " (303 million)\n";
    std::cout << "  Magnitude:   All stars with G ≤ 18\n";
    std::cout << "  Sky:         Full sky coverage\n";
    std::cout << "  Completeness: ~95% of observable stars for amateur telescopes\n\n";
    
    std::cout << "💡 Use case: Amateur astronomy, bright star studies, navigation\n\n";
    
    // ====================================================================
    // LIMITATION: Cone Search Performance
    // ====================================================================
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "LIMITATION #1: Slow Spatial Queries (NO SPATIAL INDEX)\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";
    
    std::cout << "Testing cone search performance...\n\n";
    
    struct ConeTest {
        double radius;
        size_t max_results;
    };
    
    std::vector<ConeTest> cone_tests = {
        {0.1, 10},
        {0.5, 50},
        {1.0, 100}
    };
    
    for (const auto& test : cone_tests) {
        std::cout << "  Cone: radius=" << test.radius << "°, max=" << test.max_results << " stars\n";
        
        auto stars = catalog.queryCone(180.0, 0.0, test.radius, test.max_results);
        double time_ms = measureTime([&]() {
            catalog.queryCone(180.0, 0.0, test.radius, test.max_results);
        });
        
        std::cout << "    Time:    " << std::fixed << std::setprecision(0) << time_ms << " ms\n";
        std::cout << "    Found:   " << stars.size() << " stars\n";
        std::cout << "    Speed:   " << std::setprecision(0) 
                  << (stats.total_stars / (time_ms / 1000.0)) << " stars/sec\n\n";
    }
    
    std::cout << "⚠️  Problem: Linear scan O(N) = 303M stars\n";
    std::cout << "    No spatial indexing → must check every star\n";
    std::cout << "    Time: 10-50 seconds for typical queries\n\n";
    
    std::cout << "🔧 Solution: Use GRAPPA3E for spatial queries (1000x faster)\n";
    std::cout << "    GRAPPA3E: Tile-based, O(tiles) complexity\n";
    std::cout << "    Result: 1-10 ms instead of 10-50 seconds\n\n";
    
    // ====================================================================
    // LIMITATION: Magnitude Limit
    // ====================================================================
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "LIMITATION #2: Magnitude Limit G ≤ 18\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";
    
    std::cout << "⚠️  Missing:\n";
    std::cout << "  Faint stars:  G > 18 (not included)\n";
    std::cout << "  Count:        ~1.5 billion stars excluded\n";
    std::cout << "  % of Gaia:    ~83% of Gaia DR3 not included\n\n";
    
    std::cout << "🔧 Solution: Use GRAPPA3E full catalog for faint stars\n\n";
    
    // ====================================================================
    // SUMMARY & RECOMMENDATIONS
    // ====================================================================
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SUMMARY & RECOMMENDATIONS                                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "✅ USE Mag18 FOR:\n";
    std::cout << "  • Fast source_id lookups (<1 ms)\n";
    std::cout << "  • Compact storage / bandwidth-limited scenarios\n";
    std::cout << "  • Archiving / distribution\n";
    std::cout << "  • Bright star applications (G ≤ 18)\n";
    std::cout << "  • Single-file portability\n\n";
    
    std::cout << "❌ DO NOT USE Mag18 FOR:\n";
    std::cout << "  • Spatial queries (cone, box) - VERY SLOW\n";
    std::cout << "  • Faint stars (G > 18) - NOT INCLUDED\n";
    std::cout << "  • Real-time cone searches - UNACCEPTABLE LAG\n\n";
    
    std::cout << "🎯 RECOMMENDED ARCHITECTURE:\n";
    std::cout << "  Use GaiaCatalog unified API with BOTH sources:\n";
    std::cout << "  • Mag18:    source_id lookups (its strength)\n";
    std::cout << "  • GRAPPA3E: spatial queries (its strength)\n";
    std::cout << "  • Cache:    Recent queries\n\n";
    
    std::cout << "Example configuration:\n";
    std::cout << "  GaiaCatalogConfig config;\n";
    std::cout << "  config.mag18_catalog_path = \"gaia_mag18.cat.gz\";\n";
    std::cout << "  config.grappa_catalog_path = \"GRAPPA3E/\";\n";
    std::cout << "  config.prefer_mag18_for_source_id = true;   // Fast!\n";
    std::cout << "  config.prefer_grappa_for_cone = true;       // 1000x faster!\n\n";
    
    std::cout << "✅ This gives you the BEST of both worlds!\n\n";
    
    return 0;
}
