# IOC_GaiaLib

🌟 **High-Performance C++ Library for Gaia DR3 Stellar Catalog**

Query 231 million stars in milliseconds with local HEALPix-indexed catalog.

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

---

## 🚀 Quick Start

### 1. Installation

```bash
git clone https://github.com/manvalan/IOC_GaiaLib.git
cd IOC_GaiaLib
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
sudo make install
```

### 2. Use in Your Project

**CMakeLists.txt:**
```cmake
find_package(IOC_GaiaLib REQUIRED)
target_link_libraries(your_app PRIVATE IOC_GaiaLib::ioc_gaialib)
```

**Your code:**
```cpp
#include <ioc_gaialib/unified_gaia_catalog.h>

int main() {
    // Initialize catalog
    std::string home = getenv("HOME");
    ioc::gaia::UnifiedGaiaCatalog::initialize(R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
    })");
    
    auto& catalog = ioc::gaia::UnifiedGaiaCatalog::getInstance();
    
    // Query stars in region
    ioc::gaia::QueryParams params;
    params.ra_center = 83.0;      // Orion
    params.dec_center = -5.0;
    params.radius = 2.0;          // 2° radius
    params.max_magnitude = 10.0;
    
    auto stars = catalog.queryCone(params);
    
    for (const auto& star : stars) {
        std::cout << "Gaia " << star.source_id 
                  << " | RA=" << star.ra 
                  << " Dec=" << star.dec
                  << " | Mag=" << star.phot_g_mean_mag << "\n";
    }
    
    // Query by source_id
    auto sirius = catalog.queryBySourceId(2947050466531873024ULL);
    
    // Query by name (IAU official names supported)
    auto vega = catalog.queryByName("Vega");
    auto polaris = catalog.queryByName("Polaris");
    
    ioc::gaia::UnifiedGaiaCatalog::shutdown();
    return 0;
}
```

---

## 📊 Performance

| Query Type | Time | Stars |
|------------|------|-------|
| Cone 0.5° | **0.001 ms** | ~50 |
| Cone 5° | **13 ms** | ~500 |
| Cone 15° | **18 ms** | ~5,000 |
| Corridor 5°×0.2° | **40 ms** | ~1,000 |
| Corridor L-shape | **37 ms** | ~100-500 |
| By source_id | **<1 ms** | 1 |
| By name | **<0.1 ms** | 1 |

---

## 📁 Catalog Data

The library uses a local multifile catalog:

```
~/.catalog/gaia_mag18_v2_multifile/
├── metadata.dat        # 185 KB - Header + HEALPix index
└── chunks/             # 19 GB - Star data
    ├── chunk_000.dat   # 1M stars each
    ├── chunk_001.dat
    └── ... (232 chunks)
```

**Total:** 231 million stars (mag ≤ 18), 19 GB

> **Note:** The library automatically manages the catalog format. You just need to specify the path.

---

## 🔧 API Reference

### UnifiedGaiaCatalog (Main API)

```cpp
#include <ioc_gaialib/unified_gaia_catalog.h>

// Initialize
static bool initialize(const std::string& json_config);
static UnifiedGaiaCatalog& getInstance();
static void shutdown();

// Queries
std::vector<GaiaStar> queryCone(const QueryParams& params);
std::vector<GaiaStar> queryCorridor(const CorridorQueryParams& params);
std::vector<GaiaStar> queryCorridorJSON(const std::string& json_params);
std::optional<GaiaStar> queryBySourceId(uint64_t source_id);
std::optional<GaiaStar> queryByName(const std::string& name);
std::optional<GaiaStar> queryByHD(const std::string& hd_number);
std::optional<GaiaStar> queryByHipparcos(const std::string& hip_number);
std::optional<GaiaStar> queryBySAO(const std::string& sao_number);
```

### GaiaStar Structure

```cpp
struct GaiaStar {
    int64_t source_id;           // Gaia DR3 identifier
    double ra, dec;              // Position [degrees]
    double parallax;             // [mas]
    double pmra, pmdec;          // Proper motion [mas/yr]
    double phot_g_mean_mag;      // G magnitude
    double phot_bp_mean_mag;     // BP magnitude
    double phot_rp_mean_mag;     // RP magnitude
    
    std::string getDesignation();      // Best available name
    std::string getAllDesignations();  // All cross-matches
};
```

### QueryParams

```cpp
struct QueryParams {
    double ra_center;       // Center RA [degrees]
    double dec_center;      // Center Dec [degrees]
    double radius;          // Search radius [degrees]
    double max_magnitude;   // Maximum G magnitude (default: 20)
    double min_parallax;    // Minimum parallax [mas] (-1 = no limit)
};
```

### CorridorQueryParams (Corridor Search)

```cpp
struct CorridorQueryParams {
    std::vector<CelestialPoint> path;  // Path points (at least 2)
    double width;                       // Corridor half-width [degrees]
    double max_magnitude;               // Maximum G magnitude
    double min_parallax;                // Minimum parallax [mas] (-1 = no limit)
    size_t max_results;                 // Max results (0 = no limit)
    
    static CorridorQueryParams fromJSON(const std::string& json);
    std::string toJSON() const;
    bool isValid() const;
    double getPathLength() const;
};
```

**JSON format for corridor query:**
```json
{
    "path": [
        {"ra": 73.0, "dec": 20.0},
        {"ra": 74.0, "dec": 21.0}
    ],
    "width": 0.5,
    "max_magnitude": 15.0,
    "min_parallax": 0.0,
    "max_results": 1000
}
```

### JSON Configuration

```json
{
    "catalog_type": "multifile_v2",
    "multifile_directory": "~/.catalog/gaia_mag18_v2_multifile"
}
```

**Required parameters:**
- `catalog_type`: Must be `"multifile_v2"`
- `multifile_directory`: Path to the catalog directory

---

## 🌟 Star Names

451 official IAU star names are built-in:

```cpp
// By common name
catalog.queryByName("Sirius");
catalog.queryByName("Vega");
catalog.queryByName("Betelgeuse");

// By catalog designation
catalog.queryByHD("48915");      // Sirius
catalog.queryByHipparcos("32349"); // Sirius
catalog.queryBySAO("151881");    // Sirius
```

---

## 📋 Requirements

- C++17 compiler
- CMake ≥ 3.15
- libcurl
- libxml2
- zlib
- OpenMP (optional, for parallel processing)

---

## 📖 Examples

```bash
# Run demo
./examples/unified_api_demo

# Run IAU integration test
./examples/iau_integration_test
```

---

## 📜 License

MIT License - see [LICENSE](LICENSE)

---

## 👤 Author

Michele Bigi (@manvalan) - IOccultCalc Team
