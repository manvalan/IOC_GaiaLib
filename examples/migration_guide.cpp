#include <iostream>
#include "../include/ioc_gaialib/unified_gaia_catalog.h"

using namespace ioc::gaia;
using namespace std;

/**
 * @brief Migration Guide - How to replace old catalog APIs with UnifiedGaiaCatalog
 * 
 * This example shows how to migrate from old catalog access methods 
 * to the new unified API system.
 */

void old_way_example() {
    cout << "❌ OLD WAY (DEPRECATED - DO NOT USE):\n";
    cout << "──────────────────────────────────────\n\n";
    
    cout << "// Old approach - multiple different APIs:\n";
    cout << "MultiFileCatalogV2 multifile(\"/path/to/multifile\");\n";
    cout << "auto stars1 = multifile.queryCone(ra, dec, radius);\n\n";
    
    cout << "Mag18CatalogV2 compressed;\n";
    cout << "compressed.open(\"/path/to/catalog.mag18v2\");\n";
    cout << "auto stars2 = compressed.queryCone(ra, dec, radius);\n\n";
    
    cout << "GaiaClient online(GaiaRelease::DR3);\n";
    cout << "auto stars3 = online.queryCone(ra, dec, radius, mag_limit);\n\n";
    
    cout << "❌ PROBLEMS with old approach:\n";
    cout << "   • Multiple different APIs to learn\n";
    cout << "   • No concurrent access optimization\n";
    cout << "   • Manual configuration and setup\n";
    cout << "   • No unified error handling\n";
    cout << "   • No performance monitoring\n";
    cout << "   • Inconsistent parameter formats\n\n";
}

void new_way_example() {
    cout << "✅ NEW WAY (MANDATORY - USE THIS):\n";
    cout << "───────────────────────────────────\n\n";
    
    cout << "// Step 1: Create JSON configuration\n";
    string config = R"({\n";
    config += R"(    "catalog_type": "multifile_v2",\n";
    config += R"(    "multifile_directory": "/path/to/multifile",\n";
    config += R"(    "max_cached_chunks": 100,\n";
    config += R"(    "log_level": "info"\n";
    config += R"(})";
    
    cout << config << "\n\n";
    
    cout << "// Step 2: Initialize unified catalog\n";
    cout << "UnifiedGaiaCatalog::initialize(config);\n";
    cout << "auto& catalog = UnifiedGaiaCatalog::getInstance();\n\n";
    
    cout << "// Step 3: Use unified API for all queries\n";
    cout << "QueryParams params;\n";
    cout << "params.ra_center = ra;\n";
    cout << "params.dec_center = dec;\n";
    cout << "params.radius = radius;\n";
    cout << "params.max_magnitude = mag_limit;\n\n";
    
    cout << "auto stars = catalog.queryCone(params);\n\n";
    
    cout << "✅ BENEFITS of new approach:\n";
    cout << "   • Single unified API for all catalog types\n";
    cout << "   • Automatic concurrent access optimization\n";
    cout << "   • JSON-based configuration system\n";
    cout << "   • Built-in error handling and retry logic\n";
    cout << "   • Comprehensive performance monitoring\n";
    cout << "   • Consistent parameters and behavior\n";
    cout << "   • Async operations and batch queries\n";
    cout << "   • Easy switching between catalog types\n\n";
}

void migration_steps() {
    cout << "🔄 MIGRATION CHECKLIST:\n";
    cout << "─────────────────────\n\n";
    
    cout << "1. ✅ Replace old includes:\n";
    cout << "   OLD: #include \"multifile_catalog_v2.h\"\n";
    cout << "   OLD: #include \"gaia_mag18_catalog_v2.h\"\n";
    cout << "   OLD: #include \"gaia_client.h\"\n";
    cout << "   NEW: #include \"unified_gaia_catalog.h\"\n\n";
    
    cout << "2. ✅ Create JSON configuration:\n";
    cout << "   • Choose catalog type: multifile_v2, compressed_v2, online_esa\n";
    cout << "   • Set appropriate paths and parameters\n";
    cout << "   • Configure caching and performance settings\n\n";
    
    cout << "3. ✅ Replace catalog initialization:\n";
    cout << "   OLD: MultiFileCatalogV2 catalog(path);\n";
    cout << "   NEW: UnifiedGaiaCatalog::initialize(json_config);\n";
    cout << "        auto& catalog = UnifiedGaiaCatalog::getInstance();\n\n";
    
    cout << "4. ✅ Update query calls:\n";
    cout << "   OLD: catalog.queryCone(ra, dec, radius)\n";
    cout << "   NEW: QueryParams params; params.ra_center=ra; params.dec_center=dec; params.radius=radius;\n";
    cout << "        catalog.queryCone(params)\n\n";
    
    cout << "5. ✅ Add error handling:\n";
    cout << "   if (!UnifiedGaiaCatalog::initialize(config)) {\n";
    cout << "       cerr << \"Failed to initialize catalog\" << endl;\n";
    cout << "       return -1;\n";
    cout << "   }\n\n";
    
    cout << "6. ✅ Cleanup on exit:\n";
    cout << "   UnifiedGaiaCatalog::shutdown();\n\n";
}

void configuration_examples() {
    cout << "⚙️  CONFIGURATION EXAMPLES:\n";
    cout << "─────────────────────────\n\n";
    
    cout << "📁 Multi-file V2 (Best Performance):\n";
    cout << R"({
    "catalog_type": "multifile_v2",
    "multifile_directory": "/Users/user/.catalog/gaia_mag18_v2_multifile",
    "max_cached_chunks": 100,
    "log_level": "info"
})" << "\n\n";

    cout << "💾 Compressed V2:\n";
    cout << R"({
    "catalog_type": "compressed_v2",
    "compressed_file_path": "/Users/user/.catalog/gaia_mag18_v2.mag18v2",
    "log_level": "info"
})" << "\n\n";

    cout << "🌐 Online ESA:\n";
    cout << R"({
    "catalog_type": "online_esa",
    "timeout_seconds": 30,
    "log_level": "warning"
})" << "\n\n";

    cout << "🔧 Advanced Configuration:\n";
    cout << R"({
    "catalog_type": "multifile_v2",
    "multifile_directory": "/data/gaia",
    "max_cached_chunks": 200,
    "max_concurrent_requests": 16,
    "cache_directory": "~/.cache/gaia_unified",
    "max_cache_size_gb": 20,
    "log_level": "debug",
    "log_file": "/var/log/gaia_catalog.log"
})" << "\n\n";
}

int main() {
    cout << "🌟 IOC GaiaLib - Migration Guide to Unified API\n";
    cout << "═══════════════════════════════════════════════\n\n";
    
    old_way_example();
    new_way_example();
    migration_steps();
    configuration_examples();
    
    cout << "⚠️  IMPORTANT NOTICE:\n";
    cout << "─────────────────\n";
    cout << "All old catalog APIs are now DEPRECATED and will be removed in future versions.\n";
    cout << "Please migrate to UnifiedGaiaCatalog as soon as possible.\n\n";
    
    cout << "For technical support and questions, please refer to the documentation\n";
    cout << "or contact the IOC GaiaLib development team.\n\n";
    
    cout << "✅ Migration guide complete!\n";
    
    return 0;
}