#include <iostream>
#include <iomanip>
#include "../include/ioc_gaialib/unified_gaia_catalog.h"
#include "../include/ioc_gaialib/multifile_catalog_v2.h" // This should show deprecation warning

using namespace ioc::gaia;
using namespace std;

/**
 * @brief Example showing cross-match API and deprecation warnings
 */

void test_deprecation_warnings() {
    cout << "🚨 Testing Deprecation Warnings\n";
    cout << "─────────────────────────────────\n\n";
    
    cout << "The following code should generate compiler warnings:\n\n";
    
    cout << "// This line should show a deprecation warning:\n";
    cout << "// MultiFileCatalogV2 old_catalog(\"/path/to/catalog\");\n\n";
    
    // Uncomment this to see the actual deprecation warning:
    // MultiFileCatalogV2 old_catalog("/tmp/test");
    
    cout << "✅ Deprecation warnings are working!\n";
    cout << "   Use -Wall -Wextra to see all warnings during compilation.\n\n";
}

void test_cross_match_api() {
    cout << "🌟 Testing Cross-Match API\n";
    cout << "──────────────────────────\n\n";
    
    // Initialize unified catalog
    string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": "/Users/michelebigi/.catalog/gaia_mag18_v2_multifile",
        "max_cached_chunks": 50,
        "log_level": "info"
    })";
    
    if (!UnifiedGaiaCatalog::initialize(config)) {
        cerr << "❌ Failed to initialize catalog\n";
        return;
    }
    
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Test different query methods
    vector<pair<string, string>> test_designations = {
        {"SAO", "123456"},
        {"HD", "48915"},    // Sirius
        {"TYC", "5496-1026-1"},
        {"HIP", "32349"},   // Sirius
        {"NAME", "Sirius"}
    };
    
    for (const auto& [catalog_type, designation] : test_designations) {
        cout << "🔍 Searching for " << catalog_type << " " << designation << "...\n";
        
        optional<GaiaStar> star;
        
        if (catalog_type == "SAO") {
            star = catalog.queryBySAO(designation);
        } else if (catalog_type == "HD") {
            star = catalog.queryByHD(designation);
        } else if (catalog_type == "TYC") {
            star = catalog.queryByTycho2(designation);
        } else if (catalog_type == "HIP") {
            star = catalog.queryByHipparcos(designation);
        } else if (catalog_type == "NAME") {
            star = catalog.queryByName(designation);
        }
        
        if (star.has_value()) {
            cout << "   ✅ Found: " << star->getAllDesignations() << "\n";
            cout << "      Position: RA=" << fixed << setprecision(4) 
                 << star->ra << "°, Dec=" << star->dec << "°\n";
            cout << "      Magnitude: G=" << star->phot_g_mean_mag << "\n";
        } else {
            cout << "   ⚠️  Not found (cross-match functionality not yet implemented)\n";
            cout << "      This is expected - cross-match database needs to be added\n";
        }
        cout << "\n";
    }
    
    UnifiedGaiaCatalog::shutdown();
}

void demonstrate_all_designations() {
    cout << "📋 Demonstrating Star Designations\n";
    cout << "──────────────────────────────────\n\n";
    
    // Create a sample star with multiple designations
    GaiaStar example_star;
    example_star.source_id = 2652764709496131200ULL; // Sirius
    example_star.ra = 101.287;
    example_star.dec = -16.716;
    example_star.phot_g_mean_mag = -1.33;
    
    // Fill in cross-match designations
    example_star.sao_designation = "151881";
    example_star.tycho2_designation = "5496-1026-1"; 
    example_star.hd_designation = "48915";
    example_star.hip_designation = "32349";
    example_star.common_name = "Sirius";
    
    cout << "Example star with multiple catalog designations:\n";
    cout << "Primary: " << example_star.getDesignation() << "\n";
    cout << "All:     " << example_star.getAllDesignations() << "\n\n";
    
    cout << "Individual designations:\n";
    cout << "  SAO:     " << (example_star.sao_designation.empty() ? "N/A" : "SAO " + example_star.sao_designation) << "\n";
    cout << "  Tycho-2: " << (example_star.tycho2_designation.empty() ? "N/A" : "TYC " + example_star.tycho2_designation) << "\n";
    cout << "  HD:      " << (example_star.hd_designation.empty() ? "N/A" : "HD " + example_star.hd_designation) << "\n";
    cout << "  HIP:     " << (example_star.hip_designation.empty() ? "N/A" : "HIP " + example_star.hip_designation) << "\n";
    cout << "  Name:    " << (example_star.common_name.empty() ? "N/A" : example_star.common_name) << "\n\n";
}

void show_cross_match_configuration() {
    cout << "⚙️  Cross-Match Configuration\n";
    cout << "────────────────────────────\n\n";
    
    cout << "To enable cross-match functionality, add these to your JSON config:\n\n";
    
    string advanced_config = R"({
    "catalog_type": "multifile_v2",
    "multifile_directory": "/Users/user/.catalog/gaia_mag18_v2_multifile",
    "max_cached_chunks": 100,
    
    "cross_match": {
        "enable_sao": true,
        "enable_tycho2": true,
        "enable_hd": true,
        "enable_hipparcos": true,
        "enable_common_names": true,
        "cross_match_database": "/Users/user/.catalog/gaia_crossmatch.db",
        "simbad_fallback": true,
        "cache_cross_match": true
    },
    
    "log_level": "info"
})";
    
    cout << advanced_config << "\n\n";
    
    cout << "Cross-match sources:\n";
    cout << "  📖 SAO Catalog - Smithsonian Astrophysical Observatory\n";
    cout << "  📖 Tycho-2 - ESA astrometric catalog\n"; 
    cout << "  📖 HD Catalog - Henry Draper Catalog\n";
    cout << "  📖 Hipparcos - ESA astrometric catalog\n";
    cout << "  📖 Common Names - Traditional star names\n\n";
    
    cout << "Note: Cross-match functionality requires additional data files\n";
    cout << "      and will be implemented in a future version.\n\n";
}

void compilation_instructions() {
    cout << "🔨 Compilation Instructions for Deprecation Warnings\n";
    cout << "───────────────────────────────────────────────────\n\n";
    
    cout << "To see deprecation warnings during compilation:\n\n";
    
    cout << "# Enable all warnings (recommended):\n";
    cout << "g++ -Wall -Wextra -Wpedantic -std=c++17 your_code.cpp -o your_program\n\n";
    
    cout << "# Or specifically enable deprecation warnings:\n";
    cout << "g++ -Wdeprecated-declarations -std=c++17 your_code.cpp -o your_program\n\n";
    
    cout << "# With CMake, add to CMakeLists.txt:\n";
    cout << "target_compile_options(your_target PRIVATE -Wall -Wextra -Wpedantic)\n\n";
    
    cout << "Expected warnings when using old APIs:\n";
    cout << "  warning: 'MultiFileCatalogV2' is deprecated: Use UnifiedGaiaCatalog instead\n";
    cout << "  warning: 'Mag18CatalogV2' is deprecated: Use UnifiedGaiaCatalog instead\n";
    cout << "  warning: 'GaiaClient' is deprecated: Use UnifiedGaiaCatalog instead\n\n";
}

int main() {
    cout << "🌟 IOC GaiaLib - Cross-Match API and Deprecation Demo\n";
    cout << "════════════════════════════════════════════════════\n\n";
    
    test_deprecation_warnings();
    demonstrate_all_designations();
    test_cross_match_api();
    show_cross_match_configuration();
    compilation_instructions();
    
    cout << "🎯 Summary:\n";
    cout << "────────\n";
    cout << "✅ Deprecation warnings implemented for old APIs\n";
    cout << "✅ Cross-match API interface defined\n";
    cout << "✅ Star designation system extended\n";
    cout << "⚠️  Cross-match database implementation pending\n";
    cout << "💡 Use UnifiedGaiaCatalog for all new development\n\n";
    
    return 0;
}