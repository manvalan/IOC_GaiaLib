# Changelog

All notable changes to IOC_GaiaLib will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2025-11-27

### 🚀 Major Features

#### Mag18 V2 Catalog - Performance Breakthrough
- **HEALPix Spatial Index** (NSIDE=64) for 100-300x faster cone searches
- **Chunk-based Compression** (1M records/chunk) for 5x faster random access
- **Extended 80-byte Records** with proper motion, photometric errors, and RUWE
- **Performance**: Cone search 0.5° in 50ms (vs 15s in V1), 5° in 500ms (vs 48s in V1)
- **Coverage**: 303 million stars (G ≤ 18), 95% of astronomical use cases
- **Size**: 14 GB (optimized format with spatial index)

#### GRAPPA3E Local Catalog Support
- Full offline access to 1.8 billion Gaia DR3 stars
- 146 GB tile-based storage (1°×1° RA/Dec grid)
- 61,202 optimized binary files
- Cone search: 1-100ms depending on area
- Memory-efficient tile loading

#### Unified GaiaCatalog API
- Intelligent routing between data sources (V2, V1, GRAPPA3E, online)
- LRU cache (10,000 stars) for frequently accessed data
- Automatic fallback mechanism
- Query statistics and performance metrics
- Thread-safe operations

### ✨ Added

#### Core Library
- `gaia_mag18_catalog_v2.h/cpp` - Optimized V2 catalog reader
- `gaia_mag18_catalog.h/cpp` - V1 catalog reader (legacy support)
- `gaia_local_catalog.h/cpp` - GRAPPA3E full catalog reader
- `gaia_catalog.h/cpp` - Unified API with intelligent routing

#### Tools
- `build_mag18_catalog_v2.cpp` - Memory-efficient V2 catalog generator
- `build_mag18_catalog.cpp` - V1 catalog generator
- `test_mag18_catalog_v2.cpp` - Comprehensive V2 test suite with benchmarks
- `test_mag18_catalog.cpp` - V1 test suite
- `test_unified_catalog.cpp` - Unified API test suite
- `comprehensive_local_test.cpp` - GRAPPA3E full test suite
- `analyze_mag18.cpp` - Catalog analysis tool

#### Documentation
- `GAIA_MAG18_CATALOG_V2.md` - Complete V2 technical guide (700+ lines)
- `MAG18_V2_QUICKSTART.md` - Quick reference guide
- `GAIA_SYSTEM_SUMMARY.md` - Complete system overview
- `MAG18_IMPROVEMENTS.md` - Performance analysis and optimization roadmap
- `GAIA_MAG18_CATALOG.md` - V1 documentation
- `MAG18_QUICKSTART.md` - V1 quick start
- `MAG18_PERFORMANCE_ANALYSIS.md` - Detailed performance metrics

### 🔧 Changed

#### Types
- Extended `GaiaStar` struct with `bp_rp` (BP-RP color) and `ruwe` (quality indicator)
- Updated constructor and initialization in `types.cpp`

#### Build System
- Added V2 sources to CMakeLists.txt
- New example targets for V2 tools
- Updated dependencies (zlib for chunk compression)

#### README
- Added Mag18 V2 feature highlights
- Performance comparison tables
- Updated quick start guide
- Added V2 usage examples

### 📊 Performance Improvements

| Operation | V1 | V2 | Improvement |
|-----------|----|----|-------------|
| Cone search 0.5° | 15,000 ms | 50 ms | **300x faster** |
| Cone search 5° | 48,000 ms | 500 ms | **96x faster** |
| Source_id query | <1 ms | <1 ms | Unchanged (optimal) |
| Brightest N stars | 49,000 ms | <2,000 ms | **25x faster** |
| Count in cone | 49,000 ms | <1,000 ms | **50x faster** |

### 🎯 Technical Details

#### HEALPix Implementation
- NSIDE=64 (49,152 pixels total)
- ~6,000 pixels with data for G ≤ 18
- NESTED scheme for hierarchical locality
- In-memory index for instant pixel lookup
- Query disc algorithm for cone search

#### Chunk Compression
- 1M stars per chunk (~303 chunks total)
- zlib level 9 compression
- LRU cache (10 chunks = 800 MB RAM)
- Parallel decompression ready
- 40% compression ratio

#### Extended Record Format
- Added: `g_mag_error`, `bp_mag_error`, `rp_mag_error`
- Added: `parallax_error`, `pmra_error`
- Added: `ruwe` (Renormalized Unit Weight Error)
- Added: `phot_bp_n_obs`, `phot_rp_n_obs`
- Added: `healpix_pixel` (precomputed)
- Total: 80 bytes (vs 52 in V1)

### 🐛 Fixed
- Memory overflow during catalog generation (temp file approach)
- Cone search O(N) bottleneck (HEALPix indexing)
- gzseek() inefficiency (chunk-based access)
- Missing proper motion errors
- Missing photometric quality indicators

### 🔄 Migration Guide

#### From V1 to V2
```cpp
// V1 (old)
#include "ioc_gaialib/gaia_mag18_catalog.h"
Mag18Catalog catalog;
catalog.open("gaia_mag18.cat.gz");

// V2 (new)
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
Mag18CatalogV2 catalog;
catalog.open("gaia_mag18_v2.cat");
```

All V1 methods are compatible with V2. New V2-only methods:
- `getExtendedRecord()` - Full 80-byte record
- `getHEALPixPixel()` - Position to pixel
- `getPixelsInCone()` - Cone to pixels

### 📦 File Sizes

- **Mag18 V1**: 9 GB (legacy)
- **Mag18 V2**: 14 GB (recommended)
- **GRAPPA3E**: 146 GB (optional, full catalog)

### 🎓 Use Case Coverage

- **V2 Recommended For**: 95% of projects (G ≤ 18 coverage)
- **V1 Legacy**: Minimal storage scenarios
- **GRAPPA3E**: Faint stars (G > 18) or complete validation

### ⚠️ Breaking Changes
None. V1 API remains fully supported for backward compatibility.

### 📚 References
- HEALPix: https://healpix.sourceforge.io/
- Gaia DR3: https://www.cosmos.esa.int/web/gaia/dr3
- GRAPPA3E: https://ftp.imcce.fr/pub/catalogs/GRAPPA3E/

---

## [1.0.0] - 2024-XX-XX

### Added
- Initial IOC_GaiaLib implementation
- Gaia DR3/EDR3/DR2 TAP/ADQL query support
- Local HEALPix cache (NSIDE=32, 12,288 tiles)
- Smart rate limiting and retry mechanism
- CSV parser for Gaia responses
- Offline catalog compiler
- GRAPPA3E asteroid catalog integration

### Features
- Cone search, box search, custom ADQL
- Cache queries ~6000x faster (3ms vs 18s)
- Real-world tested (Aldebaran, Orion, Sirius, Pleiades)
- Modern C++17 API
- Cross-platform (macOS, Linux, Windows)

---

## Version History

- **v2.0.0** (2025-11-27): Mag18 V2 with HEALPix spatial index - 300x speedup
- **v1.0.0** (2024): Initial release with TAP/ADQL support and local cache

---

**Legend:**
- 🚀 Major Features
- ✨ Added
- 🔧 Changed
- 🐛 Fixed
- 📊 Performance
- 🎯 Technical
- 🔄 Migration
- ⚠️ Breaking Changes
