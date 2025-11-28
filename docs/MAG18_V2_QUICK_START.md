# Mag18 V2 Catalog - Quick Integration Guide

## Installation

The V2 catalog is already installed and ready to use:

```bash
~/.catalog/gaia_mag18_v2.mag18v2
```

This is a symlink to the actual file at `~/catalogs/gaia_mag18_v2.mag18v2` (9.2 GB).

## Key Features

- **231.1 million stars** with G magnitude ≤ 18
- **HEALPix spatial index** (NSIDE=64) for 100-300x faster cone searches
- **Chunk-based compression** (1M stars per chunk) for efficient storage
- **Thread-safe queries** with OpenMP parallelization support
- **On-disk format**: Only indices and chunks are in memory; data streams from disk

## Performance

| Query Type | Speed | Notes |
|-----------|-------|-------|
| source_id lookup | <1 ms | Binary search |
| Cone search 0.5° | ~50 ms | vs 15s in V1 (300x faster) |
| Cone search 5° | ~500 ms | vs 48s in V1 (96x faster) |
| Count in cone | <100 ms | No decompression needed |

## C++ Usage

### Basic Load & Query

```cpp
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"

using namespace ioc::gaia;

// Open catalog
Mag18CatalogV2 catalog("~/.catalog/gaia_mag18_v2.mag18v2");

// Get statistics
std::cout << "Stars: " << catalog.getTotalStars() << "\n";
std::cout << "Pixels: " << catalog.getNumPixels() << "\n";
std::cout << "Chunks: " << catalog.getNumChunks() << "\n";

// Query by source_id
auto star = catalog.queryBySourceId(1000000);
if (star) {
    std::cout << "RA: " << star->ra << "° Dec: " << star->dec << "\n";
}

// Cone search
auto stars = catalog.queryCone(180.0, 0.0, 1.0);  // RA, Dec, radius (degrees)
std::cout << "Found " << stars.size() << " stars\n";

// Magnitude filter
auto bright = catalog.queryConeWithMagnitude(180.0, 0.0, 5.0, 10.0, 12.0);
// RA, Dec, radius, mag_min, mag_max

// Get brightest stars
auto brightest = catalog.queryBrightest(180.0, 0.0, 5.0, 5);  // top 5 stars

// Count stars (fast)
size_t count = catalog.countInCone(180.0, 0.0, 5.0);
```

### Parallel Processing

```cpp
// Enable parallel queries (OpenMP)
catalog.setParallelProcessing(true, 4);  // 4 threads

// Queries now run in parallel
auto stars = catalog.queryCone(ra, dec, radius);
```

## Data Structure (V2)

Each star record contains:
- **Identification**: source_id, HEALPix pixel (precomputed)
- **Astrometry**: RA, Dec, parallax, proper motion (RA & Dec)
- **Photometry**: G, BP, RP magnitudes and errors, BP-RP color
- **Quality**: RUWE, observation counts for BP/RP

Total: 80 bytes per star (vs 52 in V1)

## CMakeLists.txt Integration

```cmake
# Find the library
find_package(ioc_gaialib REQUIRED)

# Link to your target
target_link_libraries(my_app PRIVATE ioc_gaialib)

# Include headers
target_include_directories(my_app PRIVATE 
    ${IOC_GAIALIB_INCLUDE_DIRS}
)
```

## File Structure

```
~/.catalog/
└── gaia_mag18_v2.mag18v2  -> ~/catalogs/gaia_mag18_v2.mag18v2

~/catalogs/
├── gaia_mag18_v2.mag18v2  (9.2 GB, main file)
├── gaia_mag18.cat.gz      (V1, ~6 GB - deprecated)
└── build_v2.log           (build log from construction)
```

## Memory Usage

The V2 catalog uses **minimal RAM** after load:

- Header: 256 bytes
- HEALPix index: ~3-5 MB (8,787 pixels)
- Chunk index: ~50 KB (232 chunks)
- **Cache**: ~320 MB (last 4 decompressed chunks, LRU eviction)

**Total: ~330 MB for the entire catalog in memory**, regardless of query size.

## Thread Safety

All queries are **thread-safe**:
- File I/O protected by mutex
- Chunk cache uses shared_mutex for concurrent reads
- OpenMP `#pragma omp parallel` support

```cpp
// Safe to call from multiple threads
#pragma omp parallel for
for (int i = 0; i < 100; ++i) {
    auto stars = catalog.queryCone(ra[i], dec[i], 1.0);
}
```

## Troubleshooting

### Slow First Query

The first query may be slower if it needs to decompress a new chunk. Subsequent queries benefit from the LRU cache.

### Out of Memory on Cone Search

Each query result is kept in memory. Use `max_results` parameter to limit:

```cpp
// Only get first 10,000 stars
auto stars = catalog.queryCone(ra, dec, radius, 10000);
```

### File Not Found

Ensure the symlink is set up:

```bash
mkdir -p ~/.catalog
ln -sf ~/catalogs/gaia_mag18_v2.mag18v2 ~/.catalog/gaia_mag18_v2.mag18v2
```

## Example Program

See `examples/quick_v2_test.cpp` or `examples/test_v2_minimal.cpp` for complete working examples.

Build and run:

```bash
cd /path/to/IOC_GaiaLib
mkdir -p build && cd build
cmake ..
cmake --build .

# Run example
./examples/quick_v2_test ~/.catalog/gaia_mag18_v2.mag18v2
```

## Questions?

Refer to:
- `include/ioc_gaialib/gaia_mag18_catalog_v2.h` — Full API reference
- `src/gaia_mag18_catalog_v2.cpp` — Implementation details
- `examples/build_mag18_catalog_v2.cpp` — How the catalog was built
