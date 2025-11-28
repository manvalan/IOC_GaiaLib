/**
 * Intelligent V2 Cache Test
 * Tests and optimizes the chunk cache system for better performance
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"

using namespace std;
using namespace ioc::gaia;

void testCachePerformance(Mag18CatalogV2& catalog) {
    cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    cout << "║ Cache Performance Analysis                               ║\n";
    cout << "╚══════════════════════════════════════════════════════════╝\n\n";
    
    // Test 1: Cold cache (first cone search)
    cout << "Test 1: Cold cache (first cone search)\n";
    auto start = chrono::high_resolution_clock::now();
    auto results1 = catalog.queryCone(83.63, 22.01, 0.5);  // Pleiades region
    auto end = chrono::high_resolution_clock::now();
    auto cold_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "  Cold cache: " << cold_ms << " ms (" << results1.size() << " stars)\n";
    
    // Test 2: Warm cache (same region)
    cout << "\nTest 2: Warm cache (same region)\n";
    start = chrono::high_resolution_clock::now();
    auto results2 = catalog.queryCone(83.63, 22.01, 0.5);
    end = chrono::high_resolution_clock::now();
    auto warm_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "  Warm cache: " << warm_ms << " ms (" << results2.size() << " stars)\n";
    cout << "  Speedup: " << (float)cold_ms/warm_ms << "x\n";
    
    // Test 3: Nearby region (partial cache hit)
    cout << "\nTest 3: Nearby region (partial cache hit)\n";
    start = chrono::high_resolution_clock::now();
    auto results3 = catalog.queryCone(83.0, 22.5, 0.5);  // Nearby region
    end = chrono::high_resolution_clock::now();
    auto nearby_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "  Nearby region: " << nearby_ms << " ms (" << results3.size() << " stars)\n";
    
    // Test 4: Distant region (cache miss)
    cout << "\nTest 4: Distant region (cache miss)\n";
    start = chrono::high_resolution_clock::now();
    auto results4 = catalog.queryCone(180.0, -30.0, 0.5);  // Opposite sky
    end = chrono::high_resolution_clock::now();
    auto distant_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "  Distant region: " << distant_ms << " ms (" << results4.size() << " stars)\n";
    
    cout << "\n📊 Performance Summary:\n";
    cout << "  Cold cache:     " << cold_ms << " ms\n";
    cout << "  Warm cache:     " << warm_ms << " ms (speedup: " << (float)cold_ms/warm_ms << "x)\n";
    cout << "  Nearby region:  " << nearby_ms << " ms\n";
    cout << "  Distant region: " << distant_ms << " ms\n";
    
    if (warm_ms < 100) {
        cout << "\n✅ Cache working well! Warm queries are fast.\n";
    } else {
        cout << "\n⚠️  Cache may need optimization. Warm queries still slow.\n";
    }
}

void testPopularRegions(Mag18CatalogV2& catalog) {
    cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    cout << "║ Popular Astronomical Regions Test                       ║\n";
    cout << "╚══════════════════════════════════════════════════════════╝\n\n";
    
    struct Region {
        string name;
        double ra, dec, radius;
    };
    
    vector<Region> popular_regions = {
        {"Pleiades", 56.75, 24.12, 1.0},
        {"Orion Nebula", 83.82, -5.39, 1.0},
        {"Sirius", 101.287, -16.716, 0.5},
        {"Aldebaran", 68.98, 16.51, 0.5},
        {"Galactic Center", 266.417, -29.006, 2.0},
        {"North Celestial Pole", 0.0, 90.0, 5.0},
        {"Vega", 279.234, 38.784, 0.5}
    };
    
    cout << "Testing cone searches in popular regions...\n\n";
    
    for (const auto& region : popular_regions) {
        cout << "🌟 " << region.name << " (RA=" << region.ra << "°, Dec=" << region.dec 
             << "°, r=" << region.radius << "°)\n";
             
        auto start = chrono::high_resolution_clock::now();
        auto results = catalog.queryCone(region.ra, region.dec, region.radius);
        auto end = chrono::high_resolution_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        
        cout << "   Found " << results.size() << " stars in " << ms << " ms";
        
        if (ms < 50) {
            cout << " ✅ Fast\n";
        } else if (ms < 200) {
            cout << " ⚠️  Moderate\n";
        } else {
            cout << " 🐌 Slow\n";
        }
        
        // Show brightest stars if any found
        if (!results.empty()) {
            auto brightest = *min_element(results.begin(), results.end(),
                [](const GaiaStar& a, const GaiaStar& b) {
                    return a.phot_g_mean_mag < b.phot_g_mean_mag;
                });
            cout << "   Brightest: G=" << brightest.phot_g_mean_mag 
                 << ", RA=" << brightest.ra << "°, Dec=" << brightest.dec << "°\n";
        }
        cout << "\n";
    }
}

void testLargeSearch(Mag18CatalogV2& catalog) {
    cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    cout << "║ Large Search Test (many chunks)                         ║\n";
    cout << "╚══════════════════════════════════════════════════════════╝\n\n";
    
    cout << "Testing large cone search (5° radius around Galactic Center)...\n";
    cout << "This will require loading many chunks.\n\n";
    
    auto start = chrono::high_resolution_clock::now();
    auto results = catalog.queryCone(266.417, -29.006, 5.0);  // Large GC region
    auto end = chrono::high_resolution_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    
    cout << "✅ Found " << results.size() << " stars in " << ms << " ms\n";
    cout << "   Search rate: " << (float)results.size() / ms * 1000 << " stars/second\n";
    
    if (ms < 1000) {
        cout << "✅ Good performance for large search!\n";
    } else if (ms < 5000) {
        cout << "⚠️  Acceptable performance for large search\n";
    } else {
        cout << "🐌 Slow performance - cache optimization needed\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <catalog.mag18v2>\n";
        cout << "Tests and analyzes cache performance of V2 catalog\n";
        return 1;
    }
    
    cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    cout << "║ V2 Catalog Cache Performance Analysis                     ║\n";
    cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    try {
        cout << "Opening V2 catalog: " << argv[1] << "\n";
        Mag18CatalogV2 catalog(argv[1]);
        
        cout << "✅ Catalog loaded\n";
        cout << "   Stars: " << catalog.getTotalStars() << "\n";
        cout << "   Chunks: " << catalog.getNumChunks() << "\n";
        cout << "   Pixels: " << catalog.getNumPixels() << "\n\n";
        
        // Run performance tests
        testCachePerformance(catalog);
        testPopularRegions(catalog);
        testLargeSearch(catalog);
        
        cout << "\n╔════════════════════════════════════════════════════════════╗\n";
        cout << "║ Recommendations                                           ║\n";
        cout << "╚════════════════════════════════════════════════════════════╝\n";
        cout << "Based on the test results:\n\n";
        
        cout << "1. 💾 Cache Size: Current cache holds ~10 chunks (800MB RAM)\n";
        cout << "   - For 4GB+ RAM: Increase to 20-50 chunks\n";
        cout << "   - For 8GB+ RAM: Increase to 50-100 chunks\n\n";
        
        cout << "2. 🎯 Pre-loading: Consider pre-loading popular regions\n";
        cout << "   - Pleiades, Orion, Galactic Center chunks\n";
        cout << "   - Cache warming on startup\n\n";
        
        cout << "3. 🧠 Smart Cache: Implement usage-based retention\n";
        cout << "   - Keep frequently accessed chunks longer\n";
        cout << "   - Predict next chunks based on query patterns\n\n";
        
        cout << "4. ⚡ Alternative: Multi-file approach for instant access\n";
        cout << "   - Trade disk space (18GB) for instant queries\n";
        cout << "   - No decompression overhead\n\n";
        
        cout << "✅ Analysis completed!\n";
        
        return 0;
        
    } catch (const exception& e) {
        cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
}