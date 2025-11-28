#include <iostream>
#include "../include/ioc_gaialib/unified_gaia_catalog.h"

// These includes should show deprecation warnings in documentation:
#include "../include/ioc_gaialib/multifile_catalog_v2.h"
#include "../include/ioc_gaialib/gaia_mag18_catalog_v2.h" 
#include "../include/ioc_gaialib/gaia_client.h"

using namespace ioc::gaia;
using namespace std;

/**
 * @brief Test file to demonstrate compiler deprecation warnings
 * 
 * This file shows how the compiler warns when using deprecated APIs.
 * Uncomment the code below to see actual warnings.
 */

int main() {
    cout << "🚨 Deprecation Warning Test\n";
    cout << "═══════════════════════════\n\n";
    
    cout << "This code should generate deprecation warnings:\n\n";
    
    // ===== UNCOMMENT THESE LINES TO SEE DEPRECATION WARNINGS =====
    
    // OLD API 1: MultiFileCatalogV2 (deprecated)
    cout << "1. MultiFileCatalogV2 usage:\n";
    cout << "   // MultiFileCatalogV2 old_multifile(\"/path/to/catalog\");\n\n";
    // MultiFileCatalogV2 old_multifile("/path/to/catalog");  // <- This would warn
    
    // OLD API 2: Mag18CatalogV2 (deprecated) 
    cout << "2. Mag18CatalogV2 usage:\n";
    cout << "   // Mag18CatalogV2 old_compressed;\n";
    cout << "   // old_compressed.open(\"/path/to/catalog.mag18v2\");\n\n";
    // Mag18CatalogV2 old_compressed;  // <- This would warn
    // old_compressed.open("/path/to/catalog.mag18v2");
    
    // OLD API 3: GaiaClient (deprecated)
    cout << "3. GaiaClient usage:\n";
    cout << "   // GaiaClient old_online(GaiaRelease::DR3);\n\n";
    // GaiaClient old_online(GaiaRelease::DR3);  // <- This would warn
    
    cout << "===== NEW API (NO WARNINGS) =====\n\n";
    
    // NEW API: UnifiedGaiaCatalog (recommended)
    cout << "4. UnifiedGaiaCatalog usage (no warnings):\n";
    string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": "/Users/michelebigi/.catalog/gaia_mag18_v2_multifile",
        "max_cached_chunks": 50
    })";
    
    if (UnifiedGaiaCatalog::initialize(config)) {
        cout << "   ✅ UnifiedGaiaCatalog initialized successfully\n";
        
        auto& catalog = UnifiedGaiaCatalog::getInstance();
        auto info = catalog.getCatalogInfo();
        cout << "   📊 Catalog: " << info.catalog_name << "\n";
        
        UnifiedGaiaCatalog::shutdown();
    } else {
        cout << "   ⚠️  Could not initialize catalog (expected if path doesn't exist)\n";
    }
    
    cout << "\n🎯 Results:\n";
    cout << "────────\n";
    cout << "✅ New UnifiedGaiaCatalog API works without warnings\n";
    cout << "⚠️  Old APIs would generate deprecation warnings if uncommented\n";
    cout << "💡 Use compiler flags -Wall -Wextra to see all warnings\n\n";
    
    cout << "To test deprecation warnings:\n";
    cout << "1. Uncomment the deprecated API lines above\n";
    cout << "2. Recompile with: make deprecation_test\n";
    cout << "3. Look for warning messages in compiler output\n\n";
    
    return 0;
}