#pragma once

#include "types.h"
#include <string>
#include <memory>
#include <atomic>

namespace ioc {
namespace gaia {

// Forward declarations
class GaiaClient;

/**
 * Cross-match identifiers from other catalogs
 */
struct CrossMatchIds {
    std::string sao;           ///< SAO catalog number
    std::string tycho2;        ///< Tycho-2 identifier (TYC1-TYC2-TYC3)
    std::string ucac4;         ///< UCAC4 identifier
    std::string hipparcos;     ///< Hipparcos catalog number
    std::string designation;   ///< Original designation if any
    
    CrossMatchIds() = default;
    
    bool hasAnyCrossMatch() const {
        return !sao.empty() || !tycho2.empty() || 
               !ucac4.empty() || !hipparcos.empty();
    }
};

/**
 * Compact star entry for local catalog (astrometry only)
 */
struct CompactStar {
    int64_t source_id;         ///< Gaia DR3 source_id
    float ra;                  ///< Right Ascension [degrees]
    float dec;                 ///< Declination [degrees]
    float parallax;            ///< Parallax [mas]
    float parallax_error;      ///< Parallax error [mas]
    float pmra;                ///< Proper motion RA [mas/yr]
    float pmdec;               ///< Proper motion Dec [mas/yr]
    float phot_g_mean_mag;     ///< G magnitude
    float phot_bp_mean_mag;    ///< BP magnitude
    float phot_rp_mean_mag;    ///< RP magnitude
    CrossMatchIds cross_ids;   ///< Cross-match identifiers
    
    CompactStar() : source_id(0), ra(0), dec(0), parallax(0), parallax_error(0),
                    pmra(0), pmdec(0), phot_g_mean_mag(0), 
                    phot_bp_mean_mag(0), phot_rp_mean_mag(0) {}
    
    // Convert from GaiaStar
    static CompactStar fromGaiaStar(const GaiaStar& star);
};

/**
 * Compilation statistics
 */
struct CompilationStats {
    size_t total_stars;          ///< Total stars processed
    size_t stars_with_crossmatch;///< Stars with at least one cross-match
    size_t healpix_tiles;        ///< Number of HEALPix tiles
    size_t total_tiles;          ///< Total number of tiles in grid
    size_t disk_size_bytes;      ///< Total disk usage
    double coverage_percent;     ///< Sky coverage percentage
    int64_t compilation_time_seconds; ///< Total compilation time
    
    CompilationStats() : total_stars(0), stars_with_crossmatch(0), 
                        healpix_tiles(0), total_tiles(0), disk_size_bytes(0), 
                        coverage_percent(0.0), compilation_time_seconds(0) {}
};

/**
 * Detailed compilation progress info
 */
struct CompilationProgress {
    int percent;                  ///< Overall progress percentage (0-100)
    std::string phase;            ///< Current phase (Downloading, Processing, Compressing, etc.)
    size_t stars_processed;       ///< Total stars processed so far
    size_t current_tile_id;       ///< Current HEALPix tile being processed
    size_t total_tiles;           ///< Total number of tiles
    size_t completed_tiles;       ///< Number of completed tiles
    double current_ra;            ///< Current RA being processed [degrees]
    double current_dec;           ///< Current Dec being processed [degrees]
    double download_speed_mbps;   ///< Current download speed [MB/s]
    int64_t eta_seconds;          ///< Estimated time remaining [seconds]
    size_t memory_used_mb;        ///< Current memory usage [MB]
    std::string message;          ///< Detailed status message
};

/**
 * Compilation progress callback
 */
using CompilationCallback = std::function<void(const CompilationProgress& progress)>;

/**
 * GaiaCatalogCompiler - Build complete offline Gaia DR3 catalog
 * 
 * Downloads entire Gaia DR3 astrometric data with cross-matches to
 * SAO, Tycho-2, UCAC4, Hipparcos catalogs. Uses efficient binary format
 * with HEALPix indexing for fast spatial queries.
 * 
 * Features:
 * - Resumable downloads (checkpoint system)
 * - Parallel tile processing
 * - Automatic retry on network errors
 * - Progress tracking
 * - Disk space management
 * 
 * Example:
 * @code
 *   GaiaCatalogCompiler compiler("/data/gaia_catalog");
 *   compiler.setMaxMagnitude(18.0);  // Limit to bright stars
 *   compiler.compile(callback);       // Start/resume compilation
 * @endcode
 */
class GaiaCatalogCompiler {
public:
    /**
     * Construct compiler
     * 
     * @param output_dir Directory for catalog files
     * @param nside HEALPix NSIDE (default: 32 for ~13.4° tiles)
     * @param release Gaia release (default: DR3)
     */
    explicit GaiaCatalogCompiler(
        const std::string& output_dir,
        int nside = 32,
        GaiaRelease release = GaiaRelease::DR3
    );
    
    ~GaiaCatalogCompiler();
    
    // No copy, allow move
    GaiaCatalogCompiler(const GaiaCatalogCompiler&) = delete;
    GaiaCatalogCompiler& operator=(const GaiaCatalogCompiler&) = delete;
    GaiaCatalogCompiler(GaiaCatalogCompiler&&) noexcept;
    GaiaCatalogCompiler& operator=(GaiaCatalogCompiler&&) noexcept;
    
    /**
     * Set Gaia client for queries
     */
    void setClient(GaiaClient* client);
    
    /**
     * Set maximum magnitude filter
     * Only stars brighter than this will be included
     * @param max_mag Maximum G magnitude (default: no limit)
     */
    void setMaxMagnitude(double max_mag);
    
    /**
     * Set whether to download cross-match data
     * @param enable True to query SAO, Tycho-2, UCAC4, Hipparcos
     */
    void setEnableCrossMatch(bool enable);
    
    /**
     * Enable/disable compression
     * Uses zlib compression to reduce disk space by ~50-70%
     * @param enable True to enable compression (default: true)
     */
    void setEnableCompression(bool enable);
    
    /**
     * Set compression level
     * @param level 0-9, where 0=no compression, 9=max compression (default: 6)
     */
    void setCompressionLevel(int level);
    
    /**
     * Enable detailed logging
     * @param log_file Path to log file (empty = no file logging)
     * @param verbose True to enable verbose console output
     */
    void setLogging(const std::string& log_file = "", bool verbose = false);
    
    /**
     * Set number of parallel workers
     * @param workers Number of concurrent tile downloads (default: 4)
     */
    void setParallelWorkers(int workers);
    
    /**
     * Start or resume catalog compilation
     * 
     * Downloads all Gaia stars tile-by-tile. Can be interrupted and
     * resumed later - progress is checkpointed after each tile.
     * 
     * @param callback Progress callback function
     * @return Compilation statistics
     * @throws GaiaException on errors
     */
    CompilationStats compile(CompilationCallback callback = nullptr);
    
    /**
     * Check if compilation is complete
     * @return true if all tiles have been downloaded
     */
    bool isComplete() const;
    
    /**
     * Get current compilation progress
     * @return Percentage complete (0-100)
     */
    double getProgress() const;
    
    /**
     * Get compilation statistics
     * @return Current stats (even if incomplete)
     */
    CompilationStats getStatistics() const;
    
    /**
     * Pause compilation
     * Will finish current tile then stop
     */
    void pause();
    
    /**
     * Resume paused compilation
     */
    void resume();
    
    /**
     * Cancel compilation
     * Stops immediately, checkpoint saved
     */
    void cancel();
    
    /**
     * Verify catalog integrity
     * Checks all tiles and cross-references
     * @return true if catalog is valid
     */
    bool verify();
    
    /**
     * Get list of completed tiles
     * @return Vector of HEALPix tile IDs
     */
    std::vector<int> getCompletedTiles() const;
    
    /**
     * Get list of pending tiles
     * @return Vector of HEALPix tile IDs
     */
    std::vector<int> getPendingTiles() const;
    
    /**
     * Estimate total compilation time
     * Based on current download speed
     * @return Estimated seconds remaining
     */
    int64_t estimateRemainingTime() const;
    
    /**
     * Estimate total disk space required
     * @param max_magnitude Maximum magnitude filter
     * @return Estimated bytes
     */
    static size_t estimateDiskSpace(double max_magnitude);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

/**
 * Query interface for compiled catalog
 * Fast binary queries on compiled catalog
 */
class CompiledCatalog {
public:
    explicit CompiledCatalog(const std::string& catalog_dir);
    ~CompiledCatalog();
    
    CompiledCatalog(const CompiledCatalog&) = delete;
    CompiledCatalog& operator=(const CompiledCatalog&) = delete;
    
    /**
     * Query stars in region
     * @return Vector of compact stars
     */
    std::vector<CompactStar> queryRegion(
        double ra_center,
        double dec_center,
        double radius,
        double max_magnitude = 99.0
    );
    
    /**
     * Query by source ID
     * @return Star if found
     */
    std::optional<CompactStar> queryBySourceId(int64_t source_id);
    
    /**
     * Query by cross-match ID
     * @param catalog "SAO", "TYC2", "UCAC4", "HIP"
     * @param id Identifier string
     * @return Star if found
     */
    std::optional<CompactStar> queryByCrossMatch(
        const std::string& catalog,
        const std::string& id
    );
    
    /**
     * Get catalog statistics
     */
    CompilationStats getStatistics() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace gaia
} // namespace ioc
