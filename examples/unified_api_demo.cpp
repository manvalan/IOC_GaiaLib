#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include "../include/ioc_gaialib/unified_gaia_catalog.h"

using namespace ioc::gaia;
using namespace std;

/**
 * @brief Example configuration files in JSON format
 */
void createExampleConfigs() {
    // Configuration for multifile V2 catalog (recommended for performance)
    string multifile_config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + string(getenv("HOME")) + R"(/.catalog/gaia_mag18_v2_multifile",
        "max_cached_chunks": 100
    })";
    
    ofstream multifile_file("config_multifile.json");
    multifile_file << multifile_config;
    multifile_file.close();
    
    // Configuration for compressed V2 catalog
    string compressed_config = R"({
        "catalog_type": "compressed_v2", 
        "compressed_file_path": ")" + string(getenv("HOME")) + R"(/.catalog/gaia_mag18_v2.mag18v2"
    })";
    
    ofstream compressed_file("config_compressed.json");
    compressed_file << compressed_config;
    compressed_file.close();
    
    // Configuration for online ESA catalog
    string online_config = R"({
        "catalog_type": "online_esa",
        "timeout_seconds": 30
    })";
    
    ofstream online_file("config_online.json");
    online_file << online_config;
    online_file.close();
    
    cout << "📁 Created example configuration files:\n";
    cout << "   - config_multifile.json (recommended)\n";
    cout << "   - config_compressed.json\n";
    cout << "   - config_online.json\n\n";
}

void testUnifiedAPI(const string& config_file) {
    cout << "🧪 Testing Unified Gaia Catalog API\n";
    cout << "═══════════════════════════════════════\n\n";
    
    // Load configuration
    ifstream file(config_file);
    if (!file) {
        cerr << "❌ Could not open config file: " << config_file << endl;
        return;
    }
    
    string json_config((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    
    // Initialize catalog
    cout << "🔄 Initializing catalog with " << config_file << "..." << endl;
    if (!UnifiedGaiaCatalog::initialize(json_config)) {
        cerr << "❌ Failed to initialize catalog" << endl;
        return;
    }
    
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Display catalog information
    auto info = catalog.getCatalogInfo();
    cout << "✅ Catalog initialized successfully\n\n";
    cout << "📊 Catalog Information:\n";
    cout << "   Name: " << info.catalog_name << "\n";
    cout << "   Version: " << info.version << "\n";
    cout << "   Total stars: " << info.total_stars << "\n";
    cout << "   Magnitude limit: " << info.magnitude_limit << "\n";
    cout << "   Online: " << (info.is_online ? "Yes" : "No") << "\n\n";
    
    // Test 1: Simple cone search
    cout << "🎯 Test 1: Simple Cone Search\n";
    cout << "────────────────────────────────\n";
    
    QueryParams params;
    params.ra_center = 0.1;
    params.dec_center = 0.1; 
    params.radius = 1.0;
    params.max_magnitude = 15.0;
    
    auto start_time = chrono::high_resolution_clock::now();
    auto stars = catalog.queryCone(params);
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "Query: RA=" << params.ra_center << "°, Dec=" << params.dec_center 
         << "°, radius=" << params.radius << "°, max_mag=" << params.max_magnitude << "\n";
    cout << "Found: " << stars.size() << " stars in " << duration.count() << " ms\n";
    
    if (!stars.empty()) {
        cout << "\nBrightest stars:\n";
        for (size_t i = 0; i < min<size_t>(5, stars.size()); i++) {
            const auto& star = stars[i];
            cout << "  " << (i+1) << ". RA=" << fixed << setprecision(4) << star.ra
                 << "°, Dec=" << star.dec << "°, Mag=" << star.phot_g_mean_mag << "\n";
        }
    }
    cout << "\n";
    
    // Test 2: Filtered search
    cout << "🔍 Test 2: Filtered Search\n";
    cout << "──────────────────────────\n";
    
    QueryParams filtered_params;
    filtered_params.ra_center = 0.1;
    filtered_params.dec_center = 0.1;
    filtered_params.radius = 2.0;
    filtered_params.max_magnitude = 12.0;
    filtered_params.min_parallax = 1.0;  // Stars closer than 1000 pc
    
    start_time = chrono::high_resolution_clock::now();
    auto filtered_stars = catalog.queryCone(filtered_params);
    end_time = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "Query: Magnitude < 12.0, parallax > 1.0 mas, radius=2°\n";
    cout << "Found: " << filtered_stars.size() << " stars in " << duration.count() << " ms\n\n";
    
    // Test 3: Async query
    cout << "⚡ Test 3: Asynchronous Query\n";
    cout << "─────────────────────────────\n";
    
    QueryParams async_params;
    async_params.ra_center = 1.0;
    async_params.dec_center = 1.0;
    async_params.radius = 1.5;
    async_params.max_magnitude = 14.0;
    
    auto progress_callback = [](int percent, const string& status) {
        cout << "Progress: " << percent << "% - " << status << "\n";
    };
    
    auto future_result = catalog.queryAsync(async_params, progress_callback);
    
    // Do other work while query runs...
    cout << "Query running asynchronously...\n";
    
    // Get results
    auto async_stars = future_result.get();
    cout << "Async query completed: " << async_stars.size() << " stars found\n\n";
    
    // Test 4: Batch query
    cout << "📦 Test 4: Batch Query\n";
    cout << "──────────────────────\n";
    
    vector<QueryParams> batch_queries;
    for (int i = 0; i < 3; i++) {
        QueryParams batch_param;
        batch_param.ra_center = i * 0.5;
        batch_param.dec_center = i * 0.5;
        batch_param.radius = 0.5;
        batch_param.max_magnitude = 14.0;
        batch_queries.push_back(batch_param);
    }
    
    start_time = chrono::high_resolution_clock::now();
    auto batch_results = catalog.batchQuery(batch_queries);
    end_time = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "Batch query of " << batch_queries.size() << " regions completed in " 
         << duration.count() << " ms:\n";
    for (size_t i = 0; i < batch_results.size(); i++) {
        cout << "  Region " << (i+1) << ": " << batch_results[i].size() << " stars\n";
    }
    cout << "\n";
    
    // Display statistics
    cout << "📈 Performance Statistics:\n";
    cout << "─────────────────────────\n";
    auto stats = catalog.getStatistics();
    cout << "Total queries: " << stats.total_queries << "\n";
    cout << "Average query time: " << stats.average_query_time_ms << " ms\n";
    cout << "Total stars returned: " << stats.total_stars_returned << "\n";
    cout << "Cache hit rate: " << stats.cache_hit_rate << "%\n";
    cout << "Memory used: " << stats.memory_used_mb << " MB\n\n";
    
    // Cleanup
    UnifiedGaiaCatalog::shutdown();
    cout << "✅ Test completed successfully!\n";
}

int main(int argc, char* argv[]) {
    cout << "🌟 IOC GaiaLib - Unified Catalog API Demo\n";
    cout << "═══════════════════════════════════════════\n\n";
    
    // Create example configurations
    createExampleConfigs();
    
    if (argc > 1) {
        // Use provided config file
        testUnifiedAPI(argv[1]);
    } else {
        // Use default multifile config
        cout << "💡 Usage: " << argv[0] << " <config_file.json>\n";
        cout << "Using default multifile configuration...\n\n";
        testUnifiedAPI("config_multifile.json");
    }
    
    return 0;
}