/**
 * Test performance of uncompressed V2 catalog
 * Fast cone search validation
 */

#include <iostream>
#include <chrono>
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"

using namespace std;
using namespace ioc::gaia;

int main() {
    cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    cout << "║ Uncompressed V2 Catalog Performance Test                  ║\n";
    cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    try {
        // Path to uncompressed catalog
        string uncompressed_path = string(getenv("HOME")) + "/.catalog/gaia_mag18_v2_uncompressed.mag18v2";
        
        auto start = chrono::high_resolution_clock::now();
        
        cout << "Opening uncompressed V2 catalog: " << uncompressed_path << "\n";
        Mag18CatalogV2 catalog(uncompressed_path);
        
        auto load_end = chrono::high_resolution_clock::now();
        auto load_ms = chrono::duration_cast<chrono::milliseconds>(load_end - start).count();
        
        cout << "✅ Catalog loaded in " << load_ms << " ms\n";
        cout << "   Total stars: " << catalog.getTotalStars() << "\n";
        cout << "   Total chunks: " << catalog.getNumChunks() << "\n";
        cout << "   Magnitude limit: " << catalog.getMagLimit() << "\n\n";

        // Test 1: Small cone search (0.5 degrees)
        cout << "Test 1: Cone search RA=83.63, Dec=22.01, radius=0.5°\n";
        auto cone_start = chrono::high_resolution_clock::now();
        
        auto results1 = catalog.queryCone(83.63, 22.01, 0.5);
        
        auto cone_end = chrono::high_resolution_clock::now();
        auto cone_ms = chrono::duration_cast<chrono::milliseconds>(cone_end - cone_start).count();
        
        cout << "✅ Found " << results1.size() << " stars in " << cone_ms << " ms\n";
        
        if (!results1.empty()) {
            auto& star = results1[0];
            cout << "   First star: ID=" << star.source_id 
                 << ", RA=" << star.ra << "°, Dec=" << star.dec << "°, G=" << star.phot_g_mean_mag << "\n";
        }
        cout << "\n";

        // Test 2: Larger cone search (5.0 degrees)
        cout << "Test 2: Cone search RA=83.63, Dec=22.01, radius=5.0°\n";
        auto cone2_start = chrono::high_resolution_clock::now();
        
        auto results2 = catalog.queryCone(83.63, 22.01, 5.0);
        
        auto cone2_end = chrono::high_resolution_clock::now();
        auto cone2_ms = chrono::duration_cast<chrono::milliseconds>(cone2_end - cone2_start).count();
        
        cout << "✅ Found " << results2.size() << " stars in " << cone2_ms << " ms\n\n";

        // Test 3: Source ID lookup
        if (!results1.empty()) {
            auto test_id = results1[0].source_id;
            cout << "Test 3: Source ID lookup for ID=" << test_id << "\n";
            
            auto id_start = chrono::high_resolution_clock::now();
            auto star_opt = catalog.queryBySourceId(test_id);
            auto id_end = chrono::high_resolution_clock::now();
            auto id_ms = chrono::duration_cast<chrono::milliseconds>(id_end - id_start).count();
            
            if (star_opt) {
                cout << "✅ Star found in " << id_ms << " ms\n";
                cout << "   Star: RA=" << star_opt->ra << "°, Dec=" << star_opt->dec << "°, G=" << star_opt->phot_g_mean_mag << "\n";
            } else {
                cout << "❌ Star not found\n";
            }
        }

        cout << "\n╔════════════════════════════════════════════════════════════╗\n";
        cout << "║ Performance Summary                                        ║\n";
        cout << "╚════════════════════════════════════════════════════════════╝\n";
        cout << "Load time:     " << load_ms << " ms\n";
        cout << "Cone 0.5°:     " << cone_ms << " ms\n";
        cout << "Cone 5.0°:     " << cone2_ms << " ms\n";
        cout << "\n✅ All tests completed successfully!\n";
        
        return 0;

    } catch (const std::exception& e) {
        cout << "❌ Error: " << e.what() << "\n";
        return 1;
    }
}