# IOC_GaiaLib - Documentation Index

## 📖 Start Here

### New Users - Read in Order:

1. **[README.md](../README.md)** 
   - Quick start and overview
   - Installation instructions
   - Simple examples

2. **[API_GUIDE.md](API_GUIDE.md)** ⭐ **MAIN REFERENCE**
   - Complete API documentation
   - All data structures
   - Comprehensive examples
   - Best practices

3. **[MIGRATION_GUIDE.md](../MIGRATION_GUIDE.md)**
   - Migrating from old APIs
   - New features overview
   - Performance improvements

---

## 🎯 Quick Links

| Need | Read This |
|------|-----------|
| **Get started quickly** | [README.md](../README.md) |
| **Complete API reference** | [API_GUIDE.md](API_GUIDE.md) |
| **Migrate from old code** | [MIGRATION_GUIDE.md](../MIGRATION_GUIDE.md) |
| **Catalog download** | [CATALOG_DOWNLOADS.md](../CATALOG_DOWNLOADS.md) |
| **Integration examples** | [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) |

---

## 📚 Core Documentation

### Essential Files

- **[API_GUIDE.md](API_GUIDE.md)** - Primary API documentation
  - UnifiedGaiaCatalog interface
  - Query methods (cone, corridor, by ID, by name)
  - Data structures (GaiaStar, QueryParams, CorridorQueryParams)
  - Complete examples

- **[README.md](../README.md)** - Project overview
  - Quick start
  - Installation
  - Performance benchmarks

- **[MIGRATION_GUIDE.md](../MIGRATION_GUIDE.md)** - Upgrade guide
  - From deprecated APIs to UnifiedGaiaCatalog
  - New corridor query feature
  - Performance improvements

---

## 🔧 Implementation Details

### Advanced Topics

- **[UNIFIED_API.md](UNIFIED_API.md)** - Technical API details
  - Async operations
  - Batch queries
  - Performance tuning

- **[INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)** - Project integration
  - CMake setup
  - Build instructions
  - Troubleshooting

- **[THREAD_SAFETY.md](THREAD_SAFETY.md)** - Concurrency
  - Thread-safe operations
  - Multi-threaded patterns

---

## 📊 Catalog Information

- **[CATALOG_DOWNLOADS.md](../CATALOG_DOWNLOADS.md)** - Get catalog data
  - Download links
  - Installation instructions
  - File format details

- **[GAIA_SYSTEM_SUMMARY.md](GAIA_SYSTEM_SUMMARY.md)** - System overview
  - Gaia DR3 details
  - HEALPix indexing
  - Data structure

---

## 💡 Usage Patterns

### Common Tasks

| Task | Code Example |
|------|--------------|
| Initialize catalog | `UnifiedGaiaCatalog::initialize(config);` |
| Get instance | `auto& catalog = UnifiedGaiaCatalog::getInstance();` |
| Cone search | `catalog.queryCone(params);` |
| Corridor search | `catalog.queryCorridor(params);` |
| Query by name | `catalog.queryByName("Sirius");` |
| Query by ID | `catalog.queryBySourceId(source_id);` |

See **[API_GUIDE.md](API_GUIDE.md)** for complete examples.

---

## 🚀 Quick Example

```cpp
#include <ioc_gaialib/unified_gaia_catalog.h>
#include <iostream>

int main() {
    using namespace ioc::gaia;
    
    // Initialize
    std::string home = getenv("HOME");
    std::string config = R"({
        "catalog_path": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
    })";
    
    UnifiedGaiaCatalog::initialize(config);
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Query stars in Orion region
    QueryParams params;
    params.ra_center = 83.0;
    params.dec_center = -5.0;
    params.radius = 2.0;
    params.max_magnitude = 10.0;
    
    auto stars = catalog.queryCone(params);
    
    std::cout << "Found " << stars.size() << " stars\n";
    
    for (const auto& star : stars) {
        std::cout << "Gaia " << star.source_id 
                  << " | RA=" << star.ra 
                  << " Dec=" << star.dec
                  << " | Mag=" << star.phot_g_mean_mag << "\n";
    }
    
    UnifiedGaiaCatalog::shutdown();
    return 0;
}
```

---

## 📝 File Headers to Include

```cpp
// Main API (this is all you need)
#include <ioc_gaialib/unified_gaia_catalog.h>

// Type definitions (included automatically)
#include <ioc_gaialib/types.h>
```

---

## ⚠️ Deprecated Documentation

The following documents describe old APIs that are **no longer recommended**:

- `GAIA_MAG18_CATALOG.md` - Old single-file API
- `GAIA_MAG18_CATALOG_V2.md` - Old V2 API
- `MAG18_*.md` - Various old implementations

**Use [API_GUIDE.md](API_GUIDE.md) for current best practices.**

---

## 🆘 Getting Help

1. Check **[API_GUIDE.md](API_GUIDE.md)** for comprehensive documentation
2. Review **[MIGRATION_GUIDE.md](../MIGRATION_GUIDE.md)** for common issues
3. See `examples/` directory for working code
4. Open an issue on GitHub

---

## 📞 Support

- **GitHub**: https://github.com/manvalan/IOC_GaiaLib
- **Issues**: https://github.com/manvalan/IOC_GaiaLib/issues
- **Email**: [project maintainer]

---

**Last Updated**: November 2025
**Library Version**: Latest
**Gaia Release**: DR3 (2022)
