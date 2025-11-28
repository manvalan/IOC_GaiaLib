/**
 * Multi-File V2 Catalog Reader
 * Reads from directory structure with individual chunk files
 * Provides instant access by loading only needed chunks
 * 
 * @deprecated This API is deprecated. Use UnifiedGaiaCatalog instead.
 * @warning All direct catalog APIs will be removed in future versions.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <filesystem>
#include "ioc_gaialib/types.h"
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"

namespace ioc {
namespace gaia {

/**
 * @brief Multi-file V2 catalog reader for instant access
 * @deprecated Use UnifiedGaiaCatalog with "multifile_v2" configuration instead.
 * 
 * Directory structure:
 *   catalog_dir/
 *   ├── metadata.dat          (header + indices)
 *   └── chunks/
 *       ├── chunk_000.dat     (uncompressed star records)
 *       ├── chunk_001.dat
 *       └── ...
 * 
 * Performance:
 *   - Metadata loading: <1ms (always in memory)
 *   - Chunk loading: <50ms (disk I/O only)
 *   - Cone search: <10ms (no decompression overhead)
 *   - Cache friendly: OS file cache handles chunks
 */
class [[deprecated("Use UnifiedGaiaCatalog instead")]] MultiFileCatalogV2 {
public:
    /**
     * @brief Load catalog from multi-file directory
     * @param catalog_dir Directory containing metadata.dat and chunks/
     * @deprecated Use UnifiedGaiaCatalog::initialize() with multifile_v2 config instead
     */
    explicit MultiFileCatalogV2(const std::string& catalog_dir);
    
    /**
     * @brief Destructor
     */
    ~MultiFileCatalogV2();
    
    // Non-copyable, non-movable (due to mutex)
    MultiFileCatalogV2(const MultiFileCatalogV2&) = delete;
    MultiFileCatalogV2& operator=(const MultiFileCatalogV2&) = delete;
    MultiFileCatalogV2(MultiFileCatalogV2&&) = delete;
    MultiFileCatalogV2& operator=(MultiFileCatalogV2&&) = delete;
    
    /**
     * @brief Get total number of stars
     */
    uint64_t getTotalStars() const { return header_.total_stars; }
    
    /**
     * @brief Get number of chunks
     */
    uint64_t getNumChunks() const { return header_.total_chunks; }
    
    /**
     * @brief Get number of HEALPix pixels with data
     */
    uint32_t getNumPixels() const { return header_.num_healpix_pixels; }
    
    /**
     * @brief Get magnitude limit
     */
    double getMagLimit() const { return header_.mag_limit; }
    
    /**
     * @brief Get HEALPix pixel for coordinates
     */
    uint32_t getHEALPixPixel(double ra, double dec) const;
    
    /**
     * @brief Fast cone search (target: <10ms)
     * @param ra Right Ascension (degrees)
     * @param dec Declination (degrees) 
     * @param radius Search radius (degrees)
     * @param max_results Maximum results (0 = unlimited)
     * @return Vector of stars within cone
     */
    std::vector<GaiaStar> queryCone(double ra, double dec, double radius, 
                                    size_t max_results = 0);
    
    /**
     * @brief Cone search with magnitude filter
     */
    std::vector<GaiaStar> queryConeWithMagnitude(double ra, double dec, double radius,
                                                  double mag_min, double mag_max,
                                                  size_t max_results = 0);
    
    /**
     * @brief Query by source ID
     */
    std::optional<GaiaStar> queryBySourceId(uint64_t source_id);
    
    /**
     * @brief Pre-load chunks for popular regions (optional optimization)
     */
    void preloadPopularRegions();
    
    /**
     * @brief Get cache statistics
     */
    struct CacheStats {
        size_t chunks_loaded;
        size_t cache_hits;
        size_t cache_misses;
        size_t memory_used_mb;
    };
    CacheStats getCacheStats() const;
    
    /**
     * @brief Clear chunk cache to free memory
     */
    void clearCache();
    
private:
    std::string catalog_dir_;
    Mag18CatalogHeaderV2 header_;
    std::vector<HEALPixIndexEntry> healpix_index_;
    std::vector<ChunkInfo> chunk_index_;
    
    // Chunk cache
    struct ChunkCache {
        uint64_t chunk_id;
        std::vector<Mag18RecordV2> records;
        std::chrono::steady_clock::time_point last_access;
        
        ChunkCache(uint64_t id, std::vector<Mag18RecordV2> data) 
            : chunk_id(id), records(std::move(data)), 
              last_access(std::chrono::steady_clock::now()) {}
    };
    
    mutable std::unordered_map<uint64_t, std::unique_ptr<ChunkCache>> chunk_cache_;
    mutable std::mutex cache_mutex_;
    mutable size_t cache_hits_ = 0;
    mutable size_t cache_misses_ = 0;
    
    static constexpr size_t MAX_CACHED_CHUNKS = 20;  // ~1.6GB RAM
    
    // Internal methods
    bool loadMetadata();
    std::vector<Mag18RecordV2> loadChunk(uint64_t chunk_id);
    std::vector<uint32_t> getPixelsInCone(double ra, double dec, double radius) const;
    GaiaStar recordToStar(const Mag18RecordV2& record) const;
    double angularDistance(double ra1, double dec1, double ra2, double dec2) const;
    uint32_t ang2pix_nest(double theta, double phi) const;
    void evictLRUChunk();
    std::string getChunkPath(uint64_t chunk_id) const;
};

} // namespace gaia
} // namespace ioc