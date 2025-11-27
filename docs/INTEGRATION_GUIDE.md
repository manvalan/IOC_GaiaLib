# IOC_GaiaLib - Integration Guide

Quick guide to integrate the Gaia catalog library into your application.

## Table of Contents
- [Installation](#installation)
- [CMake Integration](#cmake-integration)
- [Basic Usage](#basic-usage)
- [Complete Examples](#complete-examples)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)

---

## Installation

### Option 1: Build and Install System-Wide

```bash
# Clone repository
git clone https://github.com/manvalan/IOC_GaiaLib.git
cd IOC_GaiaLib

# Build and install
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j8
sudo make install
```

This installs:
- Headers: `/usr/local/include/ioc_gaialib/`
- Library: `/usr/local/lib/libioc_gaialib.a`
- CMake config: `/usr/local/lib/cmake/IOC_GaiaLib/`

### Option 2: Add as Submodule (Recommended)

```bash
# In your project directory
git submodule add https://github.com/manvalan/IOC_GaiaLib.git external/IOC_GaiaLib
git submodule update --init --recursive
```

---

## CMake Integration

### Method 1: System-Installed Library

If you installed system-wide with `sudo make install`:

```cmake
# Your project's CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(MyAstroApp)

set(CMAKE_CXX_STANDARD 17)

# Find IOC_GaiaLib
find_package(IOC_GaiaLib REQUIRED)

# Your executable
add_executable(my_app main.cpp)

# Link library
target_link_libraries(my_app PRIVATE IOC_GaiaLib::ioc_gaialib)
```

### Method 2: Submodule/Subdirectory

If you added as submodule:

```cmake
# Your project's CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(MyAstroApp)

set(CMAKE_CXX_STANDARD 17)

# Add IOC_GaiaLib subdirectory
add_subdirectory(external/IOC_GaiaLib)

# Your executable
add_executable(my_app main.cpp)

# Link library
target_link_libraries(my_app PRIVATE ioc_gaialib)
```

### Method 3: Find Locally Built Library

If you built but didn't install:

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyAstroApp)

set(CMAKE_CXX_STANDARD 17)

# Point to IOC_GaiaLib build directory
set(IOC_GaiaLib_DIR "/path/to/IOC_GaiaLib/build")

# Include and link
include_directories(${IOC_GaiaLib_DIR}/../include)
link_directories(${IOC_GaiaLib_DIR})

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ioc_gaialib pthread z curl xml2 omp)
```

---

## Basic Usage

### Include Headers

```cpp
#include <ioc_gaialib/gaia_mag18_catalog_v2.h>
#include <iostream>

using namespace ioc::gaia;
```

### Initialize Catalog

```cpp
int main() {
    try {
        // Option 1: Constructor with path
        Mag18CatalogV2 catalog("path/to/gaia_mag18_v2.mag18v2");
        
        // Option 2: Default constructor + open
        Mag18CatalogV2 catalog;
        if (!catalog.open("path/to/gaia_mag18_v2.mag18v2")) {
            std::cerr << "Failed to open catalog" << std::endl;
            return 1;
        }
        
        // Ready to query!
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

### Query Stars

```cpp
// Cone search: Get all stars in 5° radius around (RA=180°, Dec=0°)
auto stars = catalog.queryCone(180.0, 0.0, 5.0);

std::cout << "Found " << stars.size() << " stars" << std::endl;

for (const auto& star : stars) {
    std::cout << "Source ID: " << star.source_id 
              << " | RA: " << star.ra 
              << " | Dec: " << star.dec 
              << " | Mag: " << star.phot_g_mean_mag << std::endl;
}
```

---

## Complete Examples

### Example 1: Simple Query Application

**File: `simple_query.cpp`**

```cpp
#include <ioc_gaialib/gaia_mag18_catalog_v2.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] 
                  << " <catalog_path> <ra> <dec> <radius>" << std::endl;
        return 1;
    }
    
    std::string catalog_path = argv[1];
    double ra = std::stod(argv[2]);
    double dec = std::stod(argv[3]);
    double radius = std::stod(argv[4]);
    
    try {
        // Open catalog
        ioc::gaia::Mag18CatalogV2 catalog(catalog_path);
        
        // Enable parallel processing (optional)
        catalog.setParallelProcessing(true, 4);
        
        // Query
        auto stars = catalog.queryCone(ra, dec, radius);
        
        std::cout << "Found " << stars.size() << " stars in "
                  << radius << "° cone around RA=" << ra 
                  << "°, Dec=" << dec << "°" << std::endl;
        
        // Print first 10
        for (size_t i = 0; i < std::min(stars.size(), size_t(10)); ++i) {
            const auto& s = stars[i];
            std::cout << i+1 << ". ID=" << s.source_id 
                      << " RA=" << s.ra << " Dec=" << s.dec 
                      << " Mag=" << s.phot_g_mean_mag << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

**CMakeLists.txt:**

```cmake
cmake_minimum_required(VERSION 3.15)
project(SimpleQuery)

set(CMAKE_CXX_STANDARD 17)

# Method 1: If installed system-wide
find_package(IOC_GaiaLib REQUIRED)
add_executable(simple_query simple_query.cpp)
target_link_libraries(simple_query PRIVATE IOC_GaiaLib::ioc_gaialib)

# OR Method 2: If submodule
# add_subdirectory(external/IOC_GaiaLib)
# add_executable(simple_query simple_query.cpp)
# target_link_libraries(simple_query PRIVATE ioc_gaialib)
```

**Build and run:**

```bash
mkdir build && cd build
cmake ..
make
./simple_query ~/catalogs/gaia_mag18_v2.mag18v2 180.0 0.0 5.0
```

---

### Example 2: Magnitude Filter Application

**File: `magnitude_filter.cpp`**

```cpp
#include <ioc_gaialib/gaia_mag18_catalog_v2.h>
#include <iostream>
#include <iomanip>

int main() {
    try {
        ioc::gaia::Mag18CatalogV2 catalog("gaia_mag18_v2.mag18v2");
        
        // Get bright stars (mag 10-12) in 10° cone
        double ra = 180.0, dec = 0.0, radius = 10.0;
        double mag_min = 10.0, mag_max = 12.0;
        
        auto stars = catalog.queryConeWithMagnitude(
            ra, dec, radius, mag_min, mag_max
        );
        
        std::cout << "Bright stars (mag " << mag_min << "-" << mag_max 
                  << ") in 10° cone:" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        
        for (const auto& star : stars) {
            std::cout << "ID: " << std::setw(19) << star.source_id
                      << " | RA: " << std::setw(10) << star.ra
                      << " | Dec: " << std::setw(10) << star.dec
                      << " | G: " << std::setw(6) << star.phot_g_mean_mag
                      << std::endl;
        }
        
        std::cout << "\nTotal: " << stars.size() << " stars" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

---

### Example 3: Multi-Position Query (Web Server)

**File: `star_server.cpp`**

```cpp
#include <ioc_gaialib/gaia_mag18_catalog_v2.h>
#include <iostream>
#include <vector>
#include <chrono>

struct QueryRequest {
    double ra, dec, radius;
};

class StarQueryServer {
private:
    ioc::gaia::Mag18CatalogV2 catalog_;
    
public:
    StarQueryServer(const std::string& catalog_path) 
        : catalog_(catalog_path) {
        // Enable parallel processing for fast queries
        catalog_.setParallelProcessing(true, 4);
    }
    
    // Process multiple queries efficiently
    std::vector<std::vector<ioc::gaia::GaiaStar>> 
    processQueries(const std::vector<QueryRequest>& queries) {
        
        std::vector<std::vector<ioc::gaia::GaiaStar>> results;
        results.reserve(queries.size());
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (const auto& q : queries) {
            results.push_back(
                catalog_.queryCone(q.ra, q.dec, q.radius)
            );
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        );
        
        std::cout << "Processed " << queries.size() << " queries in "
                  << duration.count() << " ms" << std::endl;
        
        return results;
    }
};

int main() {
    try {
        StarQueryServer server("gaia_mag18_v2.mag18v2");
        
        // Example batch queries
        std::vector<QueryRequest> queries = {
            {45.0, 30.0, 1.0},
            {120.0, -15.0, 1.0},
            {200.0, 45.0, 1.0},
            {315.0, -60.0, 1.0}
        };
        
        auto results = server.processQueries(queries);
        
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "Query " << i+1 << ": " 
                      << results[i].size() << " stars" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

---

## API Reference

### Class: `Mag18CatalogV2`

**Namespace:** `ioc::gaia`

#### Constructors

```cpp
Mag18CatalogV2();
Mag18CatalogV2(const std::string& catalog_path);
```

#### Catalog Management

```cpp
bool open(const std::string& catalog_path);
void close();
bool isOpen() const;
```

#### Query Methods

```cpp
// Cone search: All stars within radius (degrees)
std::vector<GaiaStar> queryCone(
    double ra,          // Right Ascension (degrees)
    double dec,         // Declination (degrees)
    double radius,      // Search radius (degrees)
    size_t max_results = 0  // 0 = unlimited
);

// Cone search with magnitude filter
std::vector<GaiaStar> queryConeWithMagnitude(
    double ra, 
    double dec, 
    double radius,
    double mag_min,     // Minimum G magnitude
    double mag_max,     // Maximum G magnitude
    size_t max_results = 0
);

// Get brightest N stars in cone
std::vector<GaiaStar> queryBrightest(
    double ra,
    double dec,
    double radius,
    size_t num_stars    // Number of brightest stars
);

// Count stars (fast, no data retrieval)
size_t countInCone(double ra, double dec, double radius);

// Query by Gaia source_id
std::optional<GaiaStar> queryBySourceId(uint64_t source_id);
```

#### Parallel Processing

```cpp
void setParallelProcessing(
    bool enable,              // Enable/disable parallelization
    size_t num_threads = 0    // 0 = auto-detect
);

bool isParallelEnabled() const;
size_t getNumThreads() const;
```

#### Catalog Info

```cpp
uint64_t getTotalStars() const;
uint32_t getHEALPixNside() const;
```

### Struct: `GaiaStar`

**Fields:**

```cpp
struct GaiaStar {
    uint64_t source_id;          // Gaia DR3 source identifier
    double ra;                   // Right Ascension (deg)
    double dec;                  // Declination (deg)
    float parallax;              // Parallax (mas)
    float pmra;                  // Proper motion RA (mas/yr)
    float pmdec;                 // Proper motion Dec (mas/yr)
    float phot_g_mean_mag;       // G-band magnitude
    float phot_bp_mean_mag;      // BP-band magnitude
    float phot_rp_mean_mag;      // RP-band magnitude
};
```

---

## Error Handling

```cpp
try {
    Mag18CatalogV2 catalog("invalid_path.mag18v2");
    // If file doesn't exist, constructor throws
    
} catch (const std::runtime_error& e) {
    std::cerr << "Catalog error: " << e.what() << std::endl;
    // Handle: file not found, corrupted, wrong version
    
} catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
}

// Alternative: Check with open()
Mag18CatalogV2 catalog;
if (!catalog.open("path.mag18v2")) {
    std::cerr << "Failed to open catalog" << std::endl;
    return 1;
}
```

---

## Performance Tips

### 1. Enable Parallel Processing

```cpp
catalog.setParallelProcessing(true, 4);  // Use 4 threads
```

- Best for large queries (> 1° radius)
- Auto-detects CPU cores if `num_threads = 0`
- Thread-safe: Multiple threads can query simultaneously

### 2. Reuse Catalog Objects

```cpp
// GOOD: One catalog object for many queries
Mag18CatalogV2 catalog("gaia_mag18_v2.mag18v2");
for (auto pos : positions) {
    catalog.queryCone(pos.ra, pos.dec, 1.0);
}

// BAD: Creating new objects repeatedly
for (auto pos : positions) {
    Mag18CatalogV2 catalog("gaia_mag18_v2.mag18v2");  // ❌ SLOW!
    catalog.queryCone(pos.ra, pos.dec, 1.0);
}
```

### 3. Use Magnitude Filters

```cpp
// Faster: Filter during query
auto stars = catalog.queryConeWithMagnitude(ra, dec, 5.0, 10.0, 15.0);

// Slower: Filter after query
auto all_stars = catalog.queryCone(ra, dec, 5.0);
std::vector<GaiaStar> filtered;
for (auto& s : all_stars) {
    if (s.phot_g_mean_mag >= 10.0 && s.phot_g_mean_mag <= 15.0) {
        filtered.push_back(s);
    }
}
```

### 4. Use max_results for Top-N Queries

```cpp
// Get first 100 stars only
auto stars = catalog.queryCone(ra, dec, 10.0, 100);
```

---

## Troubleshooting

### Problem: "Could NOT find OpenMP"

**macOS:**
```bash
brew install libomp
```

**Linux:**
```bash
sudo apt-get install libomp-dev  # Debian/Ubuntu
sudo yum install libomp-devel    # RedHat/CentOS
```

**Windows:**
- MSVC: Add `/openmp` flag
- MinGW: Install `libgomp`

### Problem: "Undefined reference to libcurl/libxml2"

Install dependencies:

**macOS:**
```bash
brew install curl libxml2 zlib
```

**Linux:**
```bash
sudo apt-get install libcurl4-openssl-dev libxml2-dev zlib1g-dev
```

### Problem: Catalog file not found

```cpp
// Use absolute path
Mag18CatalogV2 catalog("/full/path/to/gaia_mag18_v2.mag18v2");

// Or check current directory
std::filesystem::path catalog_path = 
    std::filesystem::current_path() / "gaia_mag18_v2.mag18v2";
if (!std::filesystem::exists(catalog_path)) {
    std::cerr << "Catalog not found: " << catalog_path << std::endl;
}
```

### Problem: Slow queries on HDD

- Use SSD for catalog file (10x faster random access)
- Increase cache size (recompile with larger `MAX_CACHED_CHUNKS`)
- Reduce thread count for sequential I/O

### Problem: Linking errors

If you get undefined references during linking:

```cmake
# Make sure all dependencies are linked
target_link_libraries(my_app PRIVATE 
    ioc_gaialib
    Threads::Threads
    ZLIB::ZLIB
    CURL::libcurl
    LibXml2::LibXml2
    OpenMP::OpenMP_CXX
)
```

---

## Environment Variables

Control OpenMP behavior:

```bash
export OMP_NUM_THREADS=4          # Set thread count
export OMP_SCHEDULE="dynamic"     # Load balancing
export OMP_PROC_BIND=true         # Pin threads to cores
export OMP_PLACES=cores           # Thread placement
```

---

## Minimal Working Example

**Full standalone project:**

```
my_project/
├── CMakeLists.txt
├── main.cpp
└── external/
    └── IOC_GaiaLib/  (git submodule)
```

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.15)
project(MyGaiaApp)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(external/IOC_GaiaLib)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ioc_gaialib)
```

**main.cpp:**
```cpp
#include <ioc_gaialib/gaia_mag18_catalog_v2.h>
#include <iostream>

int main() {
    ioc::gaia::Mag18CatalogV2 catalog("gaia_mag18_v2.mag18v2");
    auto stars = catalog.queryCone(180.0, 0.0, 1.0);
    std::cout << "Found " << stars.size() << " stars\n";
    return 0;
}
```

**Build:**
```bash
git clone <your-repo>
cd my_project
git submodule update --init
mkdir build && cd build
cmake ..
make
./my_app
```

---

## Next Steps

1. **Download catalog**: See [CATALOG_DOWNLOADS.md](CATALOG_DOWNLOADS.md)
2. **V2 catalog guide**: See [GAIA_MAG18_CATALOG_V2.md](GAIA_MAG18_CATALOG_V2.md)
3. **Thread-safety**: See [THREAD_SAFETY.md](THREAD_SAFETY.md)
4. **Examples**: Check `examples/` directory

## Support

- GitHub Issues: https://github.com/manvalan/IOC_GaiaLib/issues
- Documentation: https://github.com/manvalan/IOC_GaiaLib/tree/main/docs

---

**Quick Reference Card:**

| Task | Code |
|------|------|
| Open catalog | `Mag18CatalogV2 catalog("path.mag18v2");` |
| Cone search | `catalog.queryCone(ra, dec, radius);` |
| Magnitude filter | `catalog.queryConeWithMagnitude(ra, dec, r, min, max);` |
| Brightest stars | `catalog.queryBrightest(ra, dec, r, N);` |
| Count stars | `catalog.countInCone(ra, dec, radius);` |
| By source ID | `catalog.queryBySourceId(source_id);` |
| Enable parallel | `catalog.setParallelProcessing(true, 4);` |
| Total stars | `catalog.getTotalStars();` |
