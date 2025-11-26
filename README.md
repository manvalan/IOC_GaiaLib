# IOC_GaiaLib

🌟 **Professional C++ Library for ESA Gaia Astronomical Catalog Interface**

Complete, high-performance library for querying and caching Gaia DR3/EDR3/DR2 stellar catalogs with local HEALPix-based storage.

## Features

- ✅ **Full TAP/ADQL Query Support** - Cone search, box search, custom ADQL queries, source ID lookup
- ✅ **Local HEALPix Cache** - Fast spatial queries with tile-based storage (NSIDE=32, 12,288 tiles)
- ✅ **Multiple Data Releases** - DR3, EDR3, DR2 support
- ✅ **Smart Rate Limiting** - Automatic retry with exponential backoff (10 queries/min default)
- ✅ **CSV Parser** - Efficient parsing of Gaia TAP responses
- ✅ **Modern C++17** - Clean API, RAII, move semantics, pImpl idiom
- ✅ **Performance** - Cache queries ~6000x faster than network queries (3ms vs 18s)
- ✅ **Real-World Tested** - Validated with Aldebaran, Orion Belt, Sirius, Pleiades data

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

- C++17 compatible compiler
- CMake >= 3.15
- libcurl
- libxml2 (for VOTable parsing)

### Build

```bash
git clone https://github.com/manvalan/IOC_GaiaLib.git
cd IOC_GaiaLib
mkdir build && cd build
cmake ..
make -j4
sudo make install
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

### GaiaClient - Direct Queries

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

## Performance Benchmarks

Real-world tests with Gaia DR3:

| Query | Network Time | Cache Time | Speedup | Stars |
|-------|-------------|-----------|---------|-------|
| Orion Belt (box) | 6.1s | - | - | 5 |
| Pleiades (cone) | 18.4s | 3ms | **6000x** | 336 |
| Sirius (source ID) | 5.8s | - | - | 1 |

**Cache benefits:**
- First query: Downloads from Gaia Archive (~6-18s)
- Subsequent queries: Local cache access (~3ms)
- Disk usage: ~0.2 KB per star (JSON format)

## Contributing

Contributions welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) first.

## License

MIT License - see [LICENSE](LICENSE) file.

## Authors

- Michele Bigi (@manvalan)
- IOccultCalc Team

## Acknowledgments

This work has made use of data from the European Space Agency (ESA) mission Gaia (https://www.cosmos.esa.int/gaia), processed by the Gaia Data Processing and Analysis Consortium (DPAC).

## Related Projects

- [IOccultCalc](https://github.com/manvalan/IOccultCalc) - Asteroid occultation predictions
- [PyGaia](https://github.com/agabrown/PyGaia) - Python Gaia utilities
