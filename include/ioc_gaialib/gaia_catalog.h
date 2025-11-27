/**
 * @file gaia_catalog.h
 * @brief Unified Gaia catalog interface - Smart router between sources
 * 
 * Provides intelligent access to Gaia data by automatically selecting
 * the best source (Mag18 compressed, GRAPPA3E full, or online archive).
 * 
 * This is the RECOMMENDED API for all Gaia queries in IOC_GaiaLib.
 * 
 * Features:
 * - Automatic source selection based on query type
 * - Fast binary search for known source_id (Mag18)
 * - Efficient spatial queries (GRAPPA3E full)
 * - Online fallback for missing data
 * - Caching and performance optimization
 * - Thread-safe operations
 */

#ifndef IOC_GAIALIB_GAIA_CATALOG_H
#define IOC_GAIALIB_GAIA_CATALOG_H

#include "types.h"
#include "gaia_mag18_catalog.h"
#include "gaia_local_catalog.h"
#include "gaia_client.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <mutex>

namespace ioc_gaialib {

/**
 * @brief Configuration for GaiaCatalog behavior
 */
struct GaiaCatalogConfig {
    // Paths
    std::string mag18_catalog_path;      ///< Path to Mag18 compressed catalog
    std::string grappa_catalog_path;     ///< Path to GRAPPA3E full catalog
    
    // Online fallback
    bool enable_online_fallback;         ///< Use Gaia Archive if data not found locally
    ioc::gaia::GaiaRelease online_release; ///< Which Gaia release to query online
    
    // Performance tuning
    bool enable_caching;                 ///< Cache recently accessed stars
    size_t cache_size;                   ///< Maximum cache entries (default 10000)
    
    // Query routing
    bool prefer_mag18_for_source_id;     ///< Use Mag18 for source_id queries (faster)
    bool prefer_grappa_for_cone;         ///< Use GRAPPA3E for cone searches (faster)
    double cone_size_threshold;          ///< Use Mag18 if radius < threshold (degrees)
    
    // Defaults
    GaiaCatalogConfig() 
        : enable_online_fallback(false)
        , online_release(ioc::gaia::GaiaRelease::DR3)
        , enable_caching(true)
        , cache_size(10000)
        , prefer_mag18_for_source_id(true)
        , prefer_grappa_for_cone(true)
        , cone_size_threshold(5.0)
    {}
};

/**
 * @brief Unified Gaia catalog interface
 * 
 * Smart router that automatically selects the best data source for each query:
 * - Mag18 compressed: Fast source_id lookup, good for bright stars
 * - GRAPPA3E full: Fast spatial queries, all magnitudes
 * - Gaia Archive online: Fallback for missing data
 * 
 * Usage:
 * @code
 * GaiaCatalogConfig config;
 * config.mag18_catalog_path = "/path/to/gaia_mag18.cat.gz";
 * config.grappa_catalog_path = "/path/to/GRAPPA3E";
 * 
 * GaiaCatalog catalog(config);
 * 
 * // Automatic routing - uses best source
 * auto star = catalog.queryStar(12345678901234);           // Mag18 (fastest)
 * auto stars = catalog.queryCone(180.0, 0.0, 0.5);         // Mag18 (small radius)
 * auto all = catalog.queryCone(180.0, 0.0, 10.0);          // GRAPPA3E (large radius)
 * auto faint = catalog.queryConeWithMagnitude(180, 0, 1, 18, 20);  // GRAPPA3E (G>18)
 * @endcode
 */
class GaiaCatalog {
public:
    /**
     * @brief Statistics about catalog usage
     */
    struct Statistics {
        size_t total_queries;
        size_t mag18_queries;
        size_t grappa_queries;
        size_t online_queries;
        size_t cache_hits;
        size_t cache_misses;
        
        size_t mag18_stars_available;
        size_t grappa_stars_available;
        
        bool mag18_loaded;
        bool grappa_loaded;
        bool online_enabled;
    };
    
    /**
     * @brief Query performance metrics
     */
    struct QueryMetrics {
        std::string source;              ///< "mag18", "grappa", "online", "cache"
        double duration_ms;              ///< Query duration in milliseconds
        size_t results_count;            ///< Number of results returned
    };
    
    /**
     * @brief Constructor with configuration
     * @param config Configuration for catalog behavior
     */
    explicit GaiaCatalog(const GaiaCatalogConfig& config);
    
    /**
     * @brief Destructor
     */
    ~GaiaCatalog();
    
    /**
     * @brief Check if catalog is ready to use
     * @return true if at least one data source is available
     */
    bool isReady() const;
    
    /**
     * @brief Get catalog statistics
     * @return Statistics structure
     */
    Statistics getStatistics() const;
    
    // ========================================================================
    // PRIMARY QUERY METHODS - Use these for all Gaia queries
    // ========================================================================
    
    /**
     * @brief Query single star by source_id (RECOMMENDED)
     * @param source_id Gaia DR3 source identifier
     * @return GaiaStar if found, std::nullopt otherwise
     * 
     * Automatically uses fastest source:
     * 1. Cache (if enabled) - O(1)
     * 2. Mag18 catalog - O(log N), <1ms
     * 3. GRAPPA3E full - O(1) if available
     * 4. Online archive - if fallback enabled
     */
    std::optional<ioc::gaia::GaiaStar> queryStar(uint64_t source_id);
    
    /**
     * @brief Query stars in cone around position (RECOMMENDED)
     * @param ra Right Ascension (degrees, J2016.0)
     * @param dec Declination (degrees, J2016.0)
     * @param radius Search radius (degrees)
     * @param max_results Maximum number of results (0 = unlimited)
     * @return Vector of stars within cone
     * 
     * Automatically uses best source based on radius:
     * - Small radius (<5°): Try Mag18 first
     * - Large radius (≥5°): Use GRAPPA3E
     * - Fallback to other source if first unavailable
     */
    std::vector<ioc::gaia::GaiaStar> queryCone(
        double ra, double dec, double radius,
        size_t max_results = 0);
    
    /**
     * @brief Query stars with magnitude filter (RECOMMENDED)
     * @param ra Right Ascension (degrees)
     * @param dec Declination (degrees)
     * @param radius Search radius (degrees)
     * @param mag_min Minimum G magnitude
     * @param mag_max Maximum G magnitude
     * @param max_results Maximum results (0 = unlimited)
     * @return Vector of stars matching criteria
     * 
     * Routing logic:
     * - If mag_max ≤ 18: Use Mag18 (all stars available)
     * - If mag_min > 18: Use GRAPPA3E (faint stars only there)
     * - Otherwise: Use GRAPPA3E (most efficient)
     */
    std::vector<ioc::gaia::GaiaStar> queryConeWithMagnitude(
        double ra, double dec, double radius,
        double mag_min, double mag_max,
        size_t max_results = 0);
    
    /**
     * @brief Find N brightest stars in region (RECOMMENDED)
     * @param ra Right Ascension (degrees)
     * @param dec Declination (degrees)
     * @param radius Search radius (degrees)
     * @param n_brightest Number of brightest stars to return
     * @return Vector of N brightest stars, sorted by magnitude
     * 
     * Uses GRAPPA3E if available (faster for large regions),
     * otherwise Mag18.
     */
    std::vector<ioc::gaia::GaiaStar> queryBrightest(
        double ra, double dec, double radius,
        size_t n_brightest);
    
    /**
     * @brief Query rectangular region
     * @param ra_min Minimum RA (degrees)
     * @param ra_max Maximum RA (degrees)
     * @param dec_min Minimum Dec (degrees)
     * @param dec_max Maximum Dec (degrees)
     * @param max_results Maximum results (0 = unlimited)
     * @return Vector of stars in box
     * 
     * Only available with GRAPPA3E full catalog.
     */
    std::vector<ioc::gaia::GaiaStar> queryBox(
        double ra_min, double ra_max,
        double dec_min, double dec_max,
        size_t max_results = 0);
    
    /**
     * @brief Count stars in cone
     * @param ra Right Ascension (degrees)
     * @param dec Declination (degrees)
     * @param radius Search radius (degrees)
     * @return Number of stars in cone
     */
    size_t countInCone(double ra, double dec, double radius);
    
    /**
     * @brief Batch query for multiple source_ids (EFFICIENT)
     * @param source_ids Vector of source identifiers
     * @return Map of source_id to GaiaStar (missing stars not included)
     * 
     * More efficient than multiple individual queries.
     * Uses cache and bulk operations when possible.
     */
    std::map<uint64_t, ioc::gaia::GaiaStar> queryStars(
        const std::vector<uint64_t>& source_ids);
    
    // ========================================================================
    // CACHE MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Clear the cache
     */
    void clearCache();
    
    /**
     * @brief Get cache statistics
     * @return Pair of (hits, misses)
     */
    std::pair<size_t, size_t> getCacheStats() const;
    
    // ========================================================================
    // DIRECT ACCESS (Advanced users)
    // ========================================================================
    
    /**
     * @brief Get direct access to Mag18 catalog
     * @return Pointer to Mag18 catalog, nullptr if not loaded
     */
    GaiaMag18Catalog* getMag18Catalog() { return mag18_catalog_.get(); }
    
    /**
     * @brief Get direct access to GRAPPA3E catalog
     * @return Pointer to GRAPPA3E catalog, nullptr if not loaded
     */
    GaiaLocalCatalog* getGrappaCatalog() { return grappa_catalog_.get(); }
    
    /**
     * @brief Get direct access to online client
     * @return Pointer to online client, nullptr if not enabled
     */
    ioc::gaia::GaiaClient* getOnlineClient() { return online_client_.get(); }
    
    // ========================================================================
    // UTILITY METHODS
    // ========================================================================
    
    /**
     * @brief Get last query metrics
     * @return Metrics from most recent query
     */
    QueryMetrics getLastQueryMetrics() const { return last_metrics_; }
    
    /**
     * @brief Enable/disable verbose logging
     * @param enable true to enable debug output
     */
    void setVerbose(bool enable) { verbose_ = enable; }

private:
    // Configuration
    GaiaCatalogConfig config_;
    
    // Data sources
    std::unique_ptr<GaiaMag18Catalog> mag18_catalog_;
    std::unique_ptr<GaiaLocalCatalog> grappa_catalog_;
    std::unique_ptr<ioc::gaia::GaiaClient> online_client_;
    
    // Cache
    mutable std::mutex cache_mutex_;
    std::map<uint64_t, ioc::gaia::GaiaStar> star_cache_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    mutable Statistics stats_;
    QueryMetrics last_metrics_;
    
    // Settings
    bool verbose_;
    
    // Helper methods
    void updateCache(uint64_t source_id, const ioc::gaia::GaiaStar& star);
    std::optional<ioc::gaia::GaiaStar> checkCache(uint64_t source_id) const;
    void logQuery(const std::string& method, const std::string& source, 
                  double duration_ms, size_t results) const;
};

} // namespace ioc_gaialib

#endif // IOC_GAIALIB_GAIA_CATALOG_H
