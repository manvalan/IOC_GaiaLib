/**
 * Quick cone search timing test on compressed V2
 */

#include <iostream>
#include <chrono>
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"

using namespace std;
using namespace ioc::gaia;

int main() {
    cout << "\n╔══════════════════════════════════════════╗\n";
    cout << "║ V2 Cone Search Timing Test (Compressed) ║\n";
    cout << "╚══════════════════════════════════════════╝\n\n";

    try {
        string catalog_path = string(getenv("HOME")) + "/.catalog/gaia_mag18_v2.mag18v2";
        
        cout << "Opening compressed V2 catalog...\n";
        Mag18CatalogV2 catalog(catalog_path);
        cout << "✅ Opened: " << catalog.getTotalStars() << " stars\n\n";

        // Small cone search in a rich region (Pleiades area)
        cout << "Testing small cone search (0.1°) in Pleiades area...\n";
        auto start = chrono::high_resolution_clock::now();
        
        auto results = catalog.queryCone(56.75, 24.12, 0.1);  // Pleiades center
        
        auto end = chrono::high_resolution_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        
        cout << "✅ Found " << results.size() << " stars in " << ms << " ms\n";
        
        if (ms > 1000) {
            cout << "⚠️  Slow performance detected (" << ms << " ms)\n";
            cout << "   This is why we need the uncompressed version!\n";
        } else {
            cout << "✅ Good performance!\n";
        }
        
        return 0;

    } catch (const std::exception& e) {
        cout << "❌ Error: " << e.what() << "\n";
        return 1;
    }
}