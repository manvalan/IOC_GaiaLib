#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <optional>
#include "ioc_gaialib/gaia_mag18_catalog.h"

using namespace ioc_gaialib;
using namespace std;

int main(int argc, char* argv[]) {
    string catalog_file = "/Users/michelebigi/.catalog/gaia_mag18_v2.mag18v2";
    if (argc > 1) {
        catalog_file = argv[1];
    }

    uint64_t target_id = 877859914797936896;

    try {
        GaiaMag18Catalog catalog(catalog_file);
        if (!catalog.isLoaded()) {
             cerr << "Failed to load catalog: " << catalog_file << endl;
             return 1;
        }

        cout << "Searching for star ID: " << target_id << " in compressed catalog " << catalog_file << "..." << endl;
        
        auto star = catalog.queryBySourceId(target_id);
        
        if (star.has_value()) {
            cout << "✨ STAR FOUND!" << endl;
            cout << "Source ID: " << star->source_id << endl;
            cout << "RA:        " << fixed << setprecision(8) << star->ra << endl;
            cout << "Dec:       " << fixed << setprecision(8) << star->dec << endl;
            cout << "Mag G:     " << star->phot_g_mean_mag << endl;
            
            if (abs(star->ra - 90.6) < 1.0) {
                cout << "⚠️ WARNING: RA is close to 90.6 as reported. Discrepancy confirmed in COMPRESSED catalog." << endl;
            }
        } else {
            cout << "❌ Star not found in compressed catalog." << endl;
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
