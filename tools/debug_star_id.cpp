#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <optional>
#include "ioc_gaialib/concurrent_multifile_catalog_v2.h"

using namespace ioc::gaia;
using namespace std;

// This tool specifically looks for source_id 877859914797936896 
// to see what RA/Dec are stored in the binary file.

int main(int argc, char* argv[]) {
    string catalog_dir = "/Users/michelebigi/.catalog/gaia_mag18_v2_multifile";
    uint64_t target_id = 3441346123630569088; // Default to user's requested star

    if (argc > 1) {
        string arg1 = argv[1];
        // simple check if it looks like a number
        if (std::all_of(arg1.begin(), arg1.end(), ::isdigit)) {
            target_id = stoull(arg1);
        } else {
            catalog_dir = arg1;
            if (argc > 2) {
                target_id = stoull(argv[2]);
            }
        }
    }

    try {
        ConcurrentMultiFileCatalogV2 catalog(catalog_dir);
        cout << "Searching for star ID: " << target_id << " in " << catalog_dir << "..." << endl;
        
        auto star = catalog.queryBySourceId(target_id);
        
        if (star.has_value()) {
            cout << "✨ STAR FOUND!" << endl;
            cout << "Source ID: " << star->source_id << endl;
            cout << "RA:        " << fixed << setprecision(8) << star->ra << endl;
            cout << "Dec:       " << fixed << setprecision(8) << star->dec << endl;
            cout << "PM RA:     " << fixed << setprecision(8) << star->pmra << endl;
            cout << "PM Dec:    " << fixed << setprecision(8) << star->pmdec << endl;
            cout << "Mag G:     " << star->phot_g_mean_mag << endl;
            
        } else {
            cout << "❌ Star not found in catalog." << endl;
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
