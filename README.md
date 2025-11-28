# IOC_GaiaLib

🌟 **Professional C++ Library for ESA Gaia Astronomical Catalog Interface**

Complete, high-performance library for querying and caching Gaia DR3/EDR3/DR2 stellar catalogs with local HEALPix-based storage.

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.15+-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![OpenMP](https://img.shields.io/badge/OpenMP-5.1-orange.svg)](https://www.openmp.org/)

---

## 🚀 Quick Start

### Installation

```bash
# Clone and build
git clone https://github.com/manvalan/IOC_GaiaLib.git
cd IOC_GaiaLib
mkdir build && cd build
cmake ..
make -j8
sudo make install  # Optional: install system-wide
```

### Integrate into Your Project

**CMakeLists.txt:**
```cmake
find_package(IOC_GaiaLib REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE IOC_GaiaLib::ioc_gaialib)
```

**main.cpp (using UnifiedGaiaCatalog - Recommended API):**
```cpp
#include <ioc_gaialib/unified_gaia_catalog.h>
#include <iostream>

int main() {
    using namespace ioc::gaia;
    
    // Configuration JSON (multifile_v2 recommended for best performance)
    std::string config = R"({
        "catalog_type": "multifile_v2",
        "catalog_path": ")" + std::string(getenv("HOME")) + R"(/.catalog/gaia_mag18_v2_multifile",
        "enable_iau_names": true,
        "cache_size_mb": 512
    })";
    
    // Initialize unified catalog
    UnifiedGaiaCatalog::initialize(config);
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Cone search (~0.001ms for 0.5°, ~13ms for 5°)
    ConeSearchParams params{180.0, 0.0, 5.0};  // RA, Dec, radius
    params.max_magnitude = 12.0;
    auto stars = catalog.queryCone(params);
    std::cout << "Found " << stars.size() << " stars\n";
    
    // Query by star name (with IAU official names)
    auto sirius = catalog.queryByName("Sirius");
    if (sirius) {
        std::cout << "Sirius: " << sirius->designation 
                  << " (mag " << sirius->phot_g_mean_mag << ")\n";
    }
    
    // Get N brightest stars
    auto brightest = catalog.queryBrightest(params, 10);
    
    return 0;
}
```

📖 **[UnifiedGaiaCatalog Guide →](docs/UNIFIED_API_GUIDE.md)** | **[Migration Guide →](MIGRATION_GUIDE.md)** | **[Complete Integration Guide →](docs/INTEGRATION_GUIDE.md)**

---

## Features

### Core Functionality
- ✅ **Full TAP/ADQL Query Support** - Cone search, box search, custom ADQL queries, source ID lookup
- ✅ **Local HEALPix Cache** - Fast spatial queries with tile-based storage (NSIDE=32, 12,288 tiles)
- ✅ **Multiple Data Releases** - DR3, EDR3, DR2 support
- ✅ **Smart Rate Limiting** - Automatic retry with exponential backoff (10 queries/min default)
- ✅ **CSV Parser** - Efficient parsing of Gaia TAP responses
- ✅ **Modern C++17** - Clean API, RAII, move semantics, pImpl idiom
- ✅ **Performance** - Cache queries ~6000x faster than network queries (3ms vs 18s)
- ✅ **Real-World Tested** - Validated with Aldebaran, Orion Belt, Sirius, Pleiades data

### Advanced Features
- 🆕 **UnifiedGaiaCatalog** - Single unified API for all catalog types
  - **JSON configuration** for easy setup
  - **451 IAU official star names** with automatic cross-matching
  - **Multifile V2 support** (0.001ms small cone, 13ms medium cone)
  - Supports: multifile_v2, compressed_v2, online_esa
  - Thread-safe singleton pattern
  - Automatic Bayer/Flamsteed/HD/HIP designation lookup

- 🆕 **Offline Catalog Compiler** - Build complete local Gaia DR3 catalog with resumable downloads
  - HEALPix tessellation with 12,288 tiles
  - zlib compression (50-70% space savings)
  - Checkpoint system for interrupted downloads
  - Detailed progress tracking with ETA
  - Binary format optimized for fast queries
  
- ⚡ **Multifile V2 Catalog** - Ultra-fast HEALPix-partitioned catalog (RECOMMENDED)
  - **231 million stars** in ~/.catalog/gaia_mag18_v2_multifile/
  - **HEALPix spatial index** (NSIDE=64) with 49,152 files
  - **Cone search 0.5°: 0.001 ms** (blazing fast!)
  - **Cone search 5°: 13 ms** (excellent performance)
  - **Cone search 15°: 18 ms** (large regions)
  - Memory-mapped files for instant access
  - **Production-ready** for real-time applications

- ⚡ **Compressed V2 Catalog** - Single-file compressed catalog (alternative)
  - **231 million stars** in 9.2 GB compressed single file
  - **Chunk compression** (1M records/chunk) for minimal memory usage
  - Cone search 5°: ~500 ms (slower than multifile)
  - Better for limited disk I/O environments
  - **Minimal RAM**: ~330 MB regardless of query size
  
- 🆕 **GRAPPA3E Integration** - Asteroid catalog from IMCCE
  - Query asteroids by Gaia source_id, number, or designation
  - Spatial queries with HEALPix indexing
  - Physical parameters (size, albedo, rotation)
  - Seamless integration with Gaia stellar data
  - ~1.5M asteroids with astrometric data

## Quick Start

```cpp
#include <ioc_gaialib/gaia_client.h>

using namespace ioc::gaia;

// Initialize client
GaiaClient client(GaiaRelease::DR3);

// Query stars around coordinates
auto stars = client.queryCone(
    67.0,    // RA (degrees)
    15.0,    // Dec (degrees)  
    0.5,     // radius (degrees)
    12.0     // max magnitude
);

// Process results
for (const auto& star : stars) {
    std::cout << "Gaia DR3 " << star.source_id 
              << " mag=" << star.phot_g_mean_mag << "\n";
}
```

## Installation

### Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake >= 3.15
- libcurl (HTTP requests)
- libxml2 (VOTable parsing)
- zlib (compression, usually pre-installed)
- p7zip (only for GRAPPA3E extraction)

### Quick Setup

```bash
# Clone repository
git clone https://github.com/manvalan/IOC_GaiaLib.git
cd IOC_GaiaLib

# Run automated setup (builds library + downloads GRAPPA3E docs)
./scripts/setup.sh

# Or manual build
mkdir build && cd build
cmake ..
make -j4
```

### Run Examples

```bash
# Basic functionality test
./build/examples/simple_test

# Cache integration test
./build/examples/integration_test

# Complete real-world test (Aldebaran, Orion, Sirius, Pleiades)
./build/examples/real_data_test
```

## API Overview

### UnifiedGaiaCatalog - Recommended API ⭐

```cpp
#include <ioc_gaialib/unified_gaia_catalog.h>
using namespace ioc::gaia;

// JSON configuration
std::string config = R"({
    "catalog_type": "multifile_v2",
    "catalog_path": "/Users/you/.catalog/gaia_mag18_v2_multifile",
    "enable_iau_names": true
})";

// Initialize (once at startup)
UnifiedGaiaCatalog::initialize(config);
auto& catalog = UnifiedGaiaCatalog::getInstance();

// Cone search (0.001ms - 18ms depending on size)
ConeSearchParams params{180.0, 0.0, 5.0};
params.max_magnitude = 12.0;
auto stars = catalog.queryCone(params);

// Query by official IAU name (451 stars supported)
auto sirius = catalog.queryByName("Sirius");
auto vega = catalog.queryByName("Vega");
auto polaris = catalog.queryByName("Polaris");

// Query by designation
auto byHD = catalog.queryByName("HD 48915");    // Sirius
auto byHIP = catalog.queryByName("HIP 32349");  // Sirius
auto byBayer = catalog.queryByName("α CMa");    // Sirius

// Get brightest stars in region
auto brightest = catalog.queryBrightest(params, 10);
```

### GaiaClient - Direct ESA Queries (Online)

```cpp
#include <ioc_gaialib/gaia_client.h>

GaiaClient client(GaiaRelease::DR3);
client.setRateLimit(10);  // 10 queries/minute

// Cone search
auto stars = client.queryCone(ra, dec, radius, maxMagnitude);

// Box search  
auto stars = client.queryBox(raMin, raMax, decMin, decMax, maxMagnitude);

// Custom ADQL query
auto stars = client.queryADQL("SELECT * FROM gaiadr3.gaia_source WHERE ...");

// Query by source IDs
auto stars = client.queryBySourceIds({2947050466531873024L}); // Sirius
```

### GaiaCache - Local Storage

```cpp
#include <ioc_gaialib/gaia_cache.h>

// Initialize local cache (HEALPix NSIDE=32)
GaiaCache cache("/path/to/cache");
cache.setClient(&client);

// Download and cache region (one time)
cache.downloadRegion(ra, dec, radius, maxMagnitude, progressCallback);

// Fast local queries (milliseconds!)
auto stars = cache.queryRegion(ra, dec, radius, maxMagnitude);

// Check coverage
bool covered = cache.isRegionCovered(ra, dec, radius);

// Cache statistics
auto stats = cache.getStatistics();
std::cout << "Coverage: " << stats.getCoveragePercent() << "%\n";
```

### GaiaStar Data Structure

```cpp
struct GaiaStar {
    int64_t source_id;           // Unique Gaia DR3 identifier
    double ra, dec;              // Position [degrees] (ICRS, J2016.0)
    double parallax;             // Parallax [mas]
    double pmra, pmdec;          // Proper motion [mas/yr]
    double phot_g_mean_mag;      // G-band magnitude
    double phot_bp_mean_mag;     // BP-band magnitude  
    double phot_rp_mean_mag;     // RP-band magnitude
    
    bool isValid() const;
    double getBpRpColor() const;
    std::string getDesignation() const;
};
```

### 🆕 GaiaCatalogCompiler - Offline Full-Sky Catalog

```cpp
#include <ioc_gaialib/catalog_compiler.h>

GaiaCatalogCompiler compiler("/output/dir", 32, GaiaRelease::DR3);
compiler.setClient(&client);
compiler.setMaxMagnitude(17.0);           // Limit to mag 17
compiler.setEnableCompression(true);      // Enable zlib compression
compiler.setCompressionLevel(6);          // Compression level 0-9
compiler.setLogging("catalog.log", true); // Detailed logging
compiler.setParallelWorkers(4);           // Parallel downloads

// Compile with progress callback
auto stats = compiler.compile([](const CompilationProgress& p) {
    std::cout << "[" << p.percent << "%] " 
              << "Tile " << p.current_tile_id 
              << " - " << p.stars_processed << " stars"
              << " - ETA: " << p.eta_seconds << "s\n";
});

// Resume interrupted compilation
compiler.resume();

// Statistics
std::cout << "Total stars: " << stats.total_stars << "\n";
std::cout << "Disk size: " << (stats.disk_size_bytes / 1e9) << " GB\n";
```

### 🆕 GrappaReader - Asteroid Catalog

```cpp
#include <ioc_gaialib/grappa_reader.h>

// Initialize GRAPPA3E reader
GrappaReader grappa("~/catalogs/GRAPPA3E");
grappa.loadIndex();

// Query by asteroid number
auto ceres = grappa.queryByNumber(1);
if (ceres) {
    std::cout << "Ceres diameter: " << ceres->diameter_km << " km\n";
    std::cout << "Gaia source_id: " << ceres->gaia_source_id << "\n";
}

// Cone search for asteroids
auto asteroids = grappa.queryCone(ra, dec, radius);

// Advanced filtering
AsteroidQuery query;
query.diameter_min_km = 100.0;     // Large asteroids only
query.h_mag_max = 10.0;            // Bright asteroids
query.only_numbered = true;        // Numbered asteroids only
auto results = grappa.query(query);

// Integration with Gaia data
GaiaAsteroidMatcher matcher(&grappa);
for (const auto& star : stars) {
    if (matcher.isAsteroid(star.source_id)) {
        auto asteroid = matcher.getAsteroidData(star.source_id);
        std::cout << "This is asteroid: " << asteroid->designation << "\n";
    }
}
```

## Performance Benchmarks

### UnifiedGaiaCatalog with Multifile V2 (Recommended)

| Query Type | Radius | Time | Stars Found |
|------------|--------|------|-------------|
| Small cone | 0.5° | **0.001 ms** | ~50 |
| Medium cone | 5° | **13 ms** | ~500 |
| Large cone | 15° | **18 ms** | ~5,000 |
| Star by name (Sirius) | - | **<1 ms** | 1 |
| IAU name lookup | - | **<0.1 ms** | 1 |

### Comparison with Other Methods

| Method | 5° Cone Search | Notes |
|--------|----------------|-------|
| **Multifile V2** | **13 ms** | ⭐ Recommended |
| Compressed V2 | 500 ms | Single file, slower |
| Online ESA | 18,000 ms | Network dependent |
| Old Cache | 3 ms | Limited coverage |

### IAU Official Stars Integration
- **451 official IAU star names** with automatic cross-matching
- **297 Bayer designations** (α, β, γ, etc.)
- **26 Flamsteed numbers** (61 Cyg, etc.)
- **333 HR catalog** cross-references
- **406 HD catalog** cross-references
- **411 HIP catalog** cross-references

## Contributing

Contributions welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) first.

## License

MIT License - see [LICENSE](LICENSE) file.

## Authors

- Michele Bigi (@manvalan)
- IOccultCalc Team

## Examples

The library includes several example programs:

- `simple_test` - Basic library functionality
- `integration_test` - Cache-client integration
- `real_data_test` - Real-world queries (Aldebaran, Orion, Sirius, Pleiades)
- `compile_catalog` - Build offline Gaia DR3 catalog with compression
- `test_grappa` - GRAPPA3E asteroid catalog integration
- `test_catalog_query` - ADQL query diagnostics
- `test_mag17` - Sample magnitude 17 stars across sky

## Documentation

- [GRAPPA3E Integration Guide](docs/GRAPPA3E_IMPLEMENTATION.md) - Asteroid catalog setup and usage
- [API Reference](docs/API.md) - Complete API documentation (coming soon)
- [Performance Guide](docs/PERFORMANCE.md) - Optimization tips (coming soon)

## Acknowledgments

This work has made use of:
- Data from the European Space Agency (ESA) mission **Gaia** (https://www.cosmos.esa.int/gaia), processed by the Gaia Data Processing and Analysis Consortium (DPAC)
- **GRAPPA3E** catalog from IMCCE (Institut de Mécanique Céleste et de Calcul des Éphémérides)

## Related Projects

- [IOccultCalc](https://github.com/manvalan/IOccultCalc) - Asteroid occultation predictions
- [PyGaia](https://github.com/agabrown/PyGaia) - Python Gaia utilities
- [GRAPPA3E](https://ftp.imcce.fr/pub/catalogs/GRAPPA3E/) - Gaia Refined Asteroid Physical Parameters Archive
