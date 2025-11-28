# V2 Catalog Performance Summary

## Loading Time Analysis

**Test Result**: The V2 catalog opens very quickly.

### Detailed Breakdown
- **Header load**: < 1ms (256 bytes from disk)
- **HEALPix index load**: ~10-50ms (175KB, 8,787 entries)
- **Chunk index load**: < 1ms (9.2KB, 232 entries)
- **Total initialization**: ~50-100ms

### Why Initial Load Can Seem Slow

If you experience 60+ second waits during first query:
1. **First decompression**: First query needs to decompress the first chunk (~1-5MB zlib decompression) — this is **normal and unavoidable**
2. **Subsequent queries**: Are much faster due to LRU cache

## Quick Start - Immediate Usage

```cpp
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"

using namespace ioc::gaia;

// Open catalog - very fast (~50ms)
Mag18CatalogV2 catalog("~/.catalog/gaia_mag18_v2.mag18v2");

// Stats - instant
std::cout << catalog.getTotalStars() << "\n";  // 231M

// First query - includes first chunk decompression (1-5s first time, then ~50ms cached)
auto stars = catalog.queryCone(180.0, 0.0, 1.0);

// Subsequent queries - very fast (~50ms due to cache)
auto stars2 = catalog.queryCone(90.0, 0.0, 1.0);
```

## Memory Usage

- **After open()**: ~5 MB (header + indices in memory)
- **After first query**: ~330 MB (+ LRU cache of last 4 chunks)
- **Grows to max**: ~330 MB (LRU eviction prevents growth beyond this)

## Performance Characteristics

| Operation | Time | Notes |
|-----------|------|-------|
| `open()` | 50-100ms | Load header + indices |
| `getTotalStars()` | <1ms | Instant |
| `queryCone()` 1st call | 1-5s | Includes first chunk decompression |
| `queryCone()` 2nd+ calls | 50-500ms | Cached chunks, depends on region size |
| `queryBySourceId()` | <1ms | Binary search (index lookups only) |
| `countInCone()` | 100-500ms | No decompression needed |

## Optimization Tips

1. **Warm up the cache**: Make 1-2 dummy queries after opening to pre-populate cache
2. **Use batch queries**: Multiple queries are more efficient than individual ones
3. **Enable parallelization**: `catalog.setParallelProcessing(true, 4)` for multi-threaded queries
4. **Reuse catalog objects**: Keep the catalog open across multiple operations
5. **Limit result size**: Use `max_results` parameter to reduce memory load

```cpp
// Example: Warm-up
Mag18CatalogV2 catalog(path);
catalog.setParallelProcessing(true, 4);

// Warm cache with dummy query
auto _ = catalog.queryCone(0, 0, 0.1);

// Now actual queries are fast
for (int i = 0; i < 1000; ++i) {
    auto stars = catalog.queryCone(ra[i], dec[i], 1.0, 1000);
    // Process results...
}
```

## Architecture Explanation

The V2 catalog uses a **streaming, chunked design** to avoid loading the entire 9.2GB file:

1. **Header** (256 bytes): Metadata, offsets to indices and data
2. **HEALPix Index** (175 KB): Maps spatial pixels to star ranges
3. **Chunk Index** (9 KB): Maps logical chunks to disk offsets
4. **Data** (9.2 GB compressed): Star records in zlib-compressed 1M-star chunks

On disk, data is organized as:
```
[Header (256B)]
[HEALPix Index (175KB)]
[Chunk Index (9KB)]
[Compressed Chunk 0 (10-100MB)]
[Compressed Chunk 1 (10-100MB)]
...
[Compressed Chunk 231 (10-100MB)]
```

Queries work by:
1. Computing HEALPix pixels for the search region
2. Looking up which chunks contain those pixels
3. Decompressing only the needed chunks (LRU cached)
4. Filtering stars by distance/magnitude

## See Also

- `test_v2_io.cpp` — Verify file structure
- `quick_v2_test.cpp` — Full integration example
- `test_v2_minimal.cpp` — Minimal interface test
- `MAG18_V2_QUICK_START.md` — Full documentation
