#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <memory>

namespace ioc {
namespace gaia {

/**
 * GRAPPA3E Asteroid data
 * (Gaia Refined Asteroid Physical Parameters Archive v3.0)
 */
struct AsteroidData {
    int64_t gaia_source_id;     ///< Gaia DR3 source_id
    int number;                 ///< Minor planet number (0 if unnumbered)
    std::string designation;    ///< Asteroid designation (e.g., "2022 AA1")
    
    // Astrometric data (from Gaia)
    double ra;                  ///< Right Ascension [degrees]
    double dec;                 ///< Declination [degrees]
    double parallax;            ///< Parallax [mas]
    double pmra;                ///< Proper motion RA [mas/yr]
    double pmdec;               ///< Proper motion Dec [mas/yr]
    
    // Physical parameters
    double h_mag;               ///< Absolute magnitude H
    double g_slope;             ///< Slope parameter G
    double diameter_km;         ///< Estimated diameter [km]
    double albedo;              ///< Geometric albedo
    double rotation_period_h;   ///< Rotation period [hours]
    
    // Photometric data
    double phot_g_mean_mag;     ///< Gaia G magnitude
    double phot_bp_mean_mag;    ///< Gaia BP magnitude
    double phot_rp_mean_mag;    ///< Gaia RP magnitude
    
    // Quality flags
    bool has_orbit;             ///< Has orbital elements
    bool has_physical;          ///< Has physical parameters
    bool has_rotation;          ///< Has rotation data
    
    AsteroidData() : gaia_source_id(0), number(0), ra(0), dec(0), parallax(0),
                     pmra(0), pmdec(0), h_mag(0), g_slope(0), diameter_km(0),
                     albedo(0), rotation_period_h(0), phot_g_mean_mag(0),
                     phot_bp_mean_mag(0), phot_rp_mean_mag(0),
                     has_orbit(false), has_physical(false), has_rotation(false) {}
};

/**
 * Query parameters for GRAPPA3E catalog
 */
struct AsteroidQuery {
    // Spatial constraints
    std::optional<double> ra_min;
    std::optional<double> ra_max;
    std::optional<double> dec_min;
    std::optional<double> dec_max;
    
    // Magnitude constraints
    std::optional<double> h_mag_min;
    std::optional<double> h_mag_max;
    std::optional<double> g_mag_min;
    std::optional<double> g_mag_max;
    
    // Physical constraints
    std::optional<double> diameter_min_km;
    std::optional<double> diameter_max_km;
    std::optional<double> albedo_min;
    std::optional<double> albedo_max;
    
    // Filter flags
    bool only_numbered;         ///< Only numbered asteroids
    bool require_physical;      ///< Require physical parameters
    bool require_rotation;      ///< Require rotation data
    
    AsteroidQuery() : only_numbered(false), require_physical(false), 
                      require_rotation(false) {}
};

/**
 * GRAPPA3E Catalog Reader
 * Reads and queries Gaia asteroid data from GRAPPA3E catalog
 */
class GrappaReader {
public:
    /**
     * Constructor
     * @param catalog_dir Directory containing GRAPPA3E data files
     */
    explicit GrappaReader(const std::string& catalog_dir);
    
    ~GrappaReader();
    
    // Non-copyable
    GrappaReader(const GrappaReader&) = delete;
    GrappaReader& operator=(const GrappaReader&) = delete;
    
    // Movable
    GrappaReader(GrappaReader&&) noexcept;
    GrappaReader& operator=(GrappaReader&&) noexcept;
    
    /**
     * Load catalog index
     * Builds HEALPix index for fast spatial queries
     */
    void loadIndex();
    
    /**
     * Query asteroids by Gaia source ID
     * @param source_id Gaia DR3 source_id
     * @return Asteroid data if found
     */
    std::optional<AsteroidData> queryBySourceId(int64_t source_id);
    
    /**
     * Query asteroids by number
     * @param number Minor planet number
     * @return Asteroid data if found
     */
    std::optional<AsteroidData> queryByNumber(int number);
    
    /**
     * Query asteroids by designation
     * @param designation Asteroid designation
     * @return Asteroid data if found
     */
    std::optional<AsteroidData> queryByDesignation(const std::string& designation);
    
    /**
     * Query asteroids in cone region
     * @param ra Right Ascension [degrees]
     * @param dec Declination [degrees]
     * @param radius Search radius [degrees]
     * @return Vector of asteroids in region
     */
    std::vector<AsteroidData> queryCone(double ra, double dec, double radius);
    
    /**
     * Query asteroids with constraints
     * @param query Query parameters
     * @return Vector of matching asteroids
     */
    std::vector<AsteroidData> query(const AsteroidQuery& query);
    
    /**
     * Get total number of asteroids in catalog
     */
    size_t getCount() const;
    
    /**
     * Get catalog statistics
     */
    struct Statistics {
        size_t total_asteroids;
        size_t numbered_asteroids;
        size_t with_physical_params;
        size_t with_rotation_data;
        double h_mag_min;
        double h_mag_max;
        double diameter_min_km;
        double diameter_max_km;
    };
    
    Statistics getStatistics() const;
    
    /**
     * Check if catalog is loaded
     */
    bool isLoaded() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

/**
 * Integrate GRAPPA3E with GaiaStar data
 * Enriches Gaia star data with asteroid information if available
 */
class GaiaAsteroidMatcher {
public:
    GaiaAsteroidMatcher(GrappaReader* grappa_reader);
    
    /**
     * Check if a Gaia source is an asteroid
     * @param source_id Gaia DR3 source_id
     * @return true if source is known asteroid
     */
    bool isAsteroid(int64_t source_id);
    
    /**
     * Get asteroid data for a Gaia source
     * @param source_id Gaia DR3 source_id
     * @return Asteroid data if available
     */
    std::optional<AsteroidData> getAsteroidData(int64_t source_id);
    
    /**
     * Enrich GaiaStar with asteroid information
     * Adds asteroid designation to star data if it's an asteroid
     * @param star Gaia star data (modified in place)
     * @return true if star is an asteroid
     */
    bool enrichStar(GaiaStar& star);

private:
    GrappaReader* reader_;
};

} // namespace gaia
} // namespace ioc
