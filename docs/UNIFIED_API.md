# IOC GaiaLib - Unified API Documentation

## 🌟 Overview

The **Unified Gaia Catalog API** is the new mandatory interface for accessing all Gaia catalogs. It replaces all previous catalog access methods with a single, consistent, and high-performance API.

## ⚠️ Important Notice

**All old catalog APIs are now DEPRECATED** and will be removed in future versions:
- ❌ `MultiFileCatalogV2` 
- ❌ `Mag18CatalogV2`
- ❌ `GaiaClient` (direct usage)
- ❌ `GaiaLocalCatalog`

**You MUST migrate** to `UnifiedGaiaCatalog` for all new development.

## ✨ Key Features

- **Single API** for all catalog types (multifile, compressed, online)
- **Automatic concurrent optimization** (718% performance improvement)
- **JSON-based configuration** system
- **Built-in error handling** and retry logic
- **Comprehensive performance monitoring**
- **Async operations** and batch queries
- **Thread-safe** by design

## 🚀 Quick Start

### 1. Include the Header

```cpp
#include "ioc_gaialib/unified_gaia_catalog.h"
using namespace ioc::gaia;
```

### 2. Create JSON Configuration

#### Multi-file V2 (Best Performance)
```json
{
    "catalog_type": "multifile_v2",
    "multifile_directory": "/path/to/multifile/catalog",
    "max_cached_chunks": 100,
    "log_level": "info"
}
```

#### Compressed V2
```json
{
    "catalog_type": "compressed_v2",
    "compressed_file_path": "/path/to/catalog.mag18v2",
    "log_level": "info"
}
```

#### Online ESA
```json
{
    "catalog_type": "online_esa",
    "timeout_seconds": 30,
    "log_level": "warning"
}
```

### 3. Initialize and Query

```cpp
#include <iostream>
#include "ioc_gaialib/unified_gaia_catalog.h"

int main() {
    // Configuration
    std::string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": "/Users/user/.catalog/gaia_mag18_v2_multifile",
        "max_cached_chunks": 100,
        "log_level": "info"
    })";
    
    // Initialize
    if (!UnifiedGaiaCatalog::initialize(config)) {
        std::cerr << "Failed to initialize catalog" << std::endl;
        return -1;
    }
    
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Query
    QueryParams params;
    params.ra_center = 67.0;      // degrees
    params.dec_center = 15.0;     // degrees  
    params.radius = 1.0;          // degrees
    params.max_magnitude = 15.0;  // G magnitude limit
    
    auto stars = catalog.queryCone(params);
    
    std::cout << "Found " << stars.size() << " stars" << std::endl;
    
    // Cleanup
    UnifiedGaiaCatalog::shutdown();
    return 0;
}
```

## 📚 API Reference

### Configuration Types

#### `GaiaCatalogConfig::CatalogType`
- `MULTIFILE_V2` - Local multi-file V2 catalog (recommended)
- `COMPRESSED_V2` - Local compressed V2 catalog  
- `ONLINE_ESA` - ESA Gaia Archive online
- `ONLINE_VIZIER` - VizieR online service

#### `GaiaCatalogConfig::CacheStrategy`
- `DISABLED` - No caching
- `MEMORY_ONLY` - In-memory cache only
- `DISK_CACHE` - Persistent disk cache
- `HYBRID` - Memory + disk cache (default)

### Query Parameters

The `QueryParams` struct from `types.h` is used for all queries:

```cpp
struct QueryParams {
    double ra_center;      // Right Ascension (degrees)
    double dec_center;     // Declination (degrees)
    double radius;         // Search radius (degrees)  
    double max_magnitude;  // Maximum G magnitude
    double min_parallax;   // Minimum parallax (mas), -1 = no limit
};
```

### Core Methods

#### Initialization
```cpp
static bool initialize(const std::string& json_config);
static UnifiedGaiaCatalog& getInstance();
static void shutdown();
```

#### Query Operations
```cpp
// Synchronous cone search
std::vector<GaiaStar> queryCone(const QueryParams& params) const;

// Asynchronous cone search  
std::future<std::vector<GaiaStar>> queryAsync(
    const QueryParams& params,
    ProgressCallback progress_callback = nullptr
) const;

// Batch queries
std::vector<std::vector<GaiaStar>> batchQuery(
    const std::vector<QueryParams>& param_list
) const;

// Query by source ID
std::optional<GaiaStar> queryBySourceId(uint64_t source_id) const;
```

#### Monitoring and Control
```cpp
CatalogInfo getCatalogInfo() const;
CatalogStats getStatistics() const;
void clearCache();
bool reconfigure(const std::string& json_config);
```

## 🔧 Advanced Configuration

### Performance Tuning
```json
{
    "catalog_type": "multifile_v2",
    "multifile_directory": "/data/gaia",
    "max_cached_chunks": 200,
    "max_concurrent_requests": 16,
    "cache_directory": "~/.cache/gaia_unified",
    "max_cache_size_gb": 20,
    "log_level": "debug",
    "log_file": "/var/log/gaia_catalog.log"
}
```

### Configuration Parameters

| Parameter | Type | Description | Default |
|-----------|------|-------------|---------|
| `catalog_type` | string | Catalog type (required) | - |
| `multifile_directory` | string | Path to multifile catalog | - |
| `compressed_file_path` | string | Path to compressed catalog | - |
| `server_url` | string | Custom server URL | - |
| `timeout_seconds` | int | Network timeout | 30 |
| `api_key` | string | API key if required | - |
| `max_cached_chunks` | int | Memory cache size | 50 |
| `max_concurrent_requests` | int | Max parallel requests | 8 |
| `cache_directory` | string | Cache location | ~/.cache/gaia_catalog |
| `max_cache_size_gb` | int | Max disk cache size | 10 |
| `log_level` | string | silent/error/warning/info/debug | warning |

## 🔄 Migration from Old APIs

### Before (Deprecated)
```cpp
// OLD WAY - Don't use this
MultiFileCatalogV2 multifile("/path/to/multifile");
auto stars1 = multifile.queryCone(ra, dec, radius);

Mag18CatalogV2 compressed;
compressed.open("/path/to/catalog.mag18v2");
auto stars2 = compressed.queryCone(ra, dec, radius);
```

### After (New Way)
```cpp
// NEW WAY - Use this
std::string config = R"({"catalog_type": "multifile_v2", "multifile_directory": "/path/to/multifile"})";
UnifiedGaiaCatalog::initialize(config);
auto& catalog = UnifiedGaiaCatalog::getInstance();

QueryParams params;
params.ra_center = ra;
params.dec_center = dec;  
params.radius = radius;
auto stars = catalog.queryCone(params);
```

## 📊 Performance Benchmarks

### Concurrent Performance (8 threads)
- **Original API**: 29 queries/sec, 2000% degradation  
- **Unified API**: 240 queries/sec, 718% improvement
- **Cache hit rate**: 94.6%
- **Average query time**: <30ms

### Memory Usage
- **Cached chunks**: ~4GB for 100 chunks
- **Shared instances**: Multiple threads share same data
- **Automatic cleanup**: LRU cache eviction

## 🐛 Error Handling

```cpp
try {
    if (!UnifiedGaiaCatalog::initialize(config)) {
        throw std::runtime_error("Catalog initialization failed");
    }
    
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    auto stars = catalog.queryCone(params);
    
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    // Handle error appropriately
}
```

## 🧪 Testing

Run the included examples:

```bash
# Build examples
make unified_api_demo migration_guide

# Test unified API
./examples/unified_api_demo

# View migration guide  
./examples/migration_guide
```

## ⚡ Async Operations

```cpp
// Async query with progress callback
auto progress_callback = [](int percent, const std::string& status) {
    std::cout << "Progress: " << percent << "% - " << status << std::endl;
};

auto future_result = catalog.queryAsync(params, progress_callback);

// Do other work...

auto stars = future_result.get(); // Get results when ready
```

## 📦 Batch Operations

```cpp
// Query multiple regions efficiently
std::vector<QueryParams> queries;
for (int i = 0; i < 10; i++) {
    QueryParams p;
    p.ra_center = i * 10.0;
    p.dec_center = i * 10.0; 
    p.radius = 1.0;
    queries.push_back(p);
}

auto results = catalog.batchQuery(queries);
// results[i] contains stars for queries[i]
```

## 🎯 Best Practices

1. **Use multifile_v2** for best performance
2. **Initialize once** per application
3. **Reuse QueryParams** for similar queries  
4. **Monitor statistics** for performance tuning
5. **Handle errors** gracefully
6. **Call shutdown()** before exit
7. **Use async queries** for long-running operations

## 📞 Support

For technical support:
- Check examples in `/examples/` directory
- Review migration guide: `./examples/migration_guide`
- Contact IOC GaiaLib development team

## 🔗 Related Files

- Header: `include/ioc_gaialib/unified_gaia_catalog.h`
- Implementation: `src/unified_gaia_catalog.cpp`
- Examples: `examples/unified_api_demo.cpp`, `examples/migration_guide.cpp`
- Legacy APIs: All other catalog headers (deprecated)