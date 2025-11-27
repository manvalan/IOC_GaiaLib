#ifndef IOC_GAIALIB_GAIA_LOCAL_CATALOG_H
#define IOC_GAIALIB_GAIA_LOCAL_CATALOG_H

#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace ioc_gaialib {

// Import types from ioc::gaia namespace
using ioc::gaia::GaiaStar;
using ioc::gaia::EquatorialCoordinates;

/**
 * @brief Binary record structure from GRAPPA3E catalog (Gaia EDR3 offline)
 * 
 * GRAPPA3E V3.0 - 52 bytes per record (as per PDF documentation)
 * 
 * Format specification from GRAPPA3E_En_V3.0.pdf:
 * - uint64 source_id (8 bytes)
 * - RA: int32 mas + uint8 sub-mas (5 bytes), uint16 error (2 bytes)
 * - Dec: int32 mas + uint8 sub-mas (5 bytes), uint16 error (2 bytes)
 * - Parallax: uint16 * 12.5µas (2 bytes), uint16 error (2 bytes)
 * - PM_RA: int32 µas/yr (4 bytes), uint16 error (2 bytes)
 * - PM_Dec: int32 µas/yr (4 bytes), uint16 error (2 bytes)
 * - RUWE: uint8 * 0.1 (1 byte)
 * - Flags: 8 bits (has_RV, has_PM, has_Hipparcos, has_Tycho, has_G/BP/RP, duplicated) (1 byte)
 * - Magnitudes G/BP/RP: uint32 compressed (3x4 = 12 bytes)
 *   Each magnitude: lower 18 bits = value * 10000, upper 14 bits = error * 10000
 * 
 * Total: 52 bytes
 * Source: https://ftp.imcce.fr/pub/catalogs/GRAPPA3E/
 */
#pragma pack(push, 1)
struct GrappaRecord {
    uint8_t raw_data[52];  // Raw binary record as per PDF specification
    
    // Parse methods to extract data from raw bytes
    uint64_t getSourceId() const;
    double getRA() const;
    double getDec() const;
    double getParallax() const;
    double getPMRA() const;
    double getPMDec() const;
    double getRUWE() const;
    std::optional<double> getMagG() const;
    std::optional<double> getMagBP() const;
    std::optional<double> getMagRP() const;
    
    // Convert to GaiaStar
    GaiaStar toGaiaStar() const;
};
#pragma pack(pop)

static_assert(sizeof(GrappaRecord) == 52, "GrappaRecord must be exactly 52 bytes (as per PDF)");

/**
 * @brief Statistics about the local Gaia catalog
 */
struct GaiaLocalStats {
    size_t total_sources{0};
    size_t sources_with_parallax{0};
    size_t sources_with_pm{0};
    size_t sources_with_rv{0};
    size_t tiles_loaded{0};
    double mag_g_min{99.0};
    double mag_g_max{0.0};
};

/**
 * @brief Local offline Gaia DR3 catalog reader (GRAPPA3E format)
 * 
 * Provides fast local access to ~1.8 billion Gaia DR3 sources without
 * network queries. Compatible interface with GaiaClient for offline work.
 * 
 * Data organization:
 * - HEALPix tiles (NSIDE=360, 1,555,200 tiles)
 * - Binary format (65 bytes/record)
 * - Total ~146 GB uncompressed
 * 
 * Usage:
 * @code
 * GaiaLocalCatalog catalog("~/catalogs/GRAPPA3E");
 * 
 * // Cone search (like GaiaClient)
 * auto stars = catalog.queryCone(83.8221, 22.0145, 2.0); // Aldebaran region
 * 
 * // Single source
 * auto sirius = catalog.queryBySourceId(2947050466531873024);
 * @endcode
 */
class GaiaLocalCatalog {
public:
    explicit GaiaLocalCatalog(const std::string& catalog_path);
    ~GaiaLocalCatalog();
    
    // Non-copyable but movable
    GaiaLocalCatalog(const GaiaLocalCatalog&) = delete;
    GaiaLocalCatalog& operator=(const GaiaLocalCatalog&) = delete;
    GaiaLocalCatalog(GaiaLocalCatalog&&) noexcept;
    GaiaLocalCatalog& operator=(GaiaLocalCatalog&&) noexcept;
    
    /**
     * @brief Query sources in a cone (compatible with GaiaClient interface)
     * @param ra_deg Right ascension in degrees (J2016.0)
     * @param dec_deg Declination in degrees (J2016.0)
     * @param radius_deg Search radius in degrees
     * @param max_results Maximum number of results (0 = unlimited)
     * @return Vector of GaiaStar objects
     */
    std::vector<GaiaStar> queryCone(
        double ra_deg,
        double dec_deg,
        double radius_deg,
        size_t max_results = 0
    ) const;
    
    /**
     * @brief Query single source by Gaia source_id
     * @param source_id Gaia DR3 source identifier
     * @return GaiaStar if found, std::nullopt otherwise
     */
    std::optional<GaiaStar> queryBySourceId(uint64_t source_id) const;
    
    /**
     * @brief Get catalog statistics
     */
    GaiaLocalStats getStatistics() const;
    
    /**
     * @brief Check if catalog is properly loaded
     */
    bool isLoaded() const;
    
    /**
     * @brief Get catalog path
     */
    std::string getCatalogPath() const;
    
    /**
     * @brief Query with magnitude filter
     * @param ra_deg Right ascension in degrees
     * @param dec_deg Declination in degrees
     * @param radius_deg Search radius in degrees
     * @param mag_min Minimum G magnitude (brighter = smaller number)
     * @param mag_max Maximum G magnitude
     * @param max_results Maximum results (0 = unlimited)
     */
    std::vector<GaiaStar> queryConeWithMagnitude(
        double ra_deg,
        double dec_deg,
        double radius_deg,
        double mag_min,
        double mag_max,
        size_t max_results = 0
    ) const;
    
    /**
     * @brief Query brightest sources in cone
     * @param ra_deg Right ascension in degrees
     * @param dec_deg Declination in degrees
     * @param radius_deg Search radius in degrees
     * @param n_brightest Number of brightest sources to return
     */
    std::vector<GaiaStar> queryBrightest(
        double ra_deg,
        double dec_deg,
        double radius_deg,
        size_t n_brightest
    ) const;
    
    /**
     * @brief Get sources in rectangular region
     * @param ra_min Minimum RA (degrees)
     * @param ra_max Maximum RA (degrees)
     * @param dec_min Minimum Dec (degrees)
     * @param dec_max Maximum Dec (degrees)
     * @param max_results Maximum results
     */
    std::vector<GaiaStar> queryBox(
        double ra_min,
        double ra_max,
        double dec_min,
        double dec_max,
        size_t max_results = 0
    ) const;
    
    /**
     * @brief Count sources in cone without loading data
     * @param ra_deg Right ascension in degrees
     * @param dec_deg Declination in degrees
     * @param radius_deg Search radius in degrees
     */
    size_t countInCone(double ra_deg, double dec_deg, double radius_deg) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace ioc_gaialib

#endif // IOC_GAIALIB_GAIA_LOCAL_CATALOG_H
