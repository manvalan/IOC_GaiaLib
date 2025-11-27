#include "ioc_gaialib/grappa_reader.h"
#include "ioc_gaialib/gaia_client.h"
#include <iostream>
#include <iomanip>

using namespace ioc::gaia;

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  GRAPPA3E Asteroid Catalog Test                     ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════╝\n" << std::endl;
    
    try {
        // Initialize GRAPPA3E reader
        std::string catalog_dir = std::string(getenv("HOME")) + "/catalogs/GRAPPA3E";
        std::cout << "📂 Loading catalog from: " << catalog_dir << std::endl;
        
        GrappaReader grappa(catalog_dir);
        
        std::cout << "⏳ Building index..." << std::endl;
        grappa.loadIndex();
        
        auto stats = grappa.getStatistics();
        
        std::cout << "\n╔══════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  Catalog Statistics                                  ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════╝" << std::endl;
        std::cout << "Total asteroids:        " << stats.total_asteroids << std::endl;
        std::cout << "Numbered asteroids:     " << stats.numbered_asteroids << std::endl;
        std::cout << "With physical params:   " << stats.with_physical_params << std::endl;
        std::cout << "With rotation data:     " << stats.with_rotation_data << std::endl;
        std::cout << "H magnitude range:      " << std::fixed << std::setprecision(2)
                  << stats.h_mag_min << " - " << stats.h_mag_max << std::endl;
        std::cout << "Diameter range:         " << std::fixed << std::setprecision(2)
                  << stats.diameter_min_km << " - " << stats.diameter_max_km << " km" << std::endl;
        
        // Test 1: Query by asteroid number (Ceres)
        std::cout << "\n[Test 1] Searching for (1) Ceres..." << std::endl;
        auto ceres = grappa.queryByNumber(1);
        if (ceres) {
            std::cout << "✅ Found:" << std::endl;
            std::cout << "   Designation: " << ceres->designation << std::endl;
            std::cout << "   Position: RA=" << std::fixed << std::setprecision(5) 
                      << ceres->ra << "°, Dec=" << ceres->dec << "°" << std::endl;
            std::cout << "   H mag: " << ceres->h_mag << std::endl;
            std::cout << "   Diameter: " << ceres->diameter_km << " km" << std::endl;
            std::cout << "   Gaia source_id: " << ceres->gaia_source_id << std::endl;
        } else {
            std::cout << "⚠️  Not found (catalog may not be fully loaded)" << std::endl;
        }
        
        // Test 2: Cone search around ecliptic plane
        std::cout << "\n[Test 2] Searching asteroids in ecliptic region..." << std::endl;
        double ra = 0.0, dec = 0.0, radius = 5.0;
        auto asteroids = grappa.queryCone(ra, dec, radius);
        std::cout << "✅ Found " << asteroids.size() << " asteroids in " 
                  << radius << "° radius" << std::endl;
        
        if (!asteroids.empty()) {
            std::cout << "\nFirst 10 asteroids:" << std::endl;
            for (size_t i = 0; i < std::min(size_t(10), asteroids.size()); ++i) {
                const auto& ast = asteroids[i];
                std::cout << "   " << (ast.number > 0 ? "(" + std::to_string(ast.number) + ") " : "")
                          << ast.designation 
                          << " - H=" << std::fixed << std::setprecision(1) << ast.h_mag
                          << ", d=" << std::fixed << std::setprecision(1) << ast.diameter_km << " km"
                          << std::endl;
            }
        }
        
        // Test 3: Query with constraints
        std::cout << "\n[Test 3] Searching large asteroids (d > 100 km)..." << std::endl;
        AsteroidQuery query;
        query.diameter_min_km = 100.0;
        query.only_numbered = true;
        
        auto large_asteroids = grappa.query(query);
        std::cout << "✅ Found " << large_asteroids.size() << " large asteroids" << std::endl;
        
        // Test 4: Integration with Gaia data
        std::cout << "\n[Test 4] Testing Gaia integration..." << std::endl;
        GaiaClient client(GaiaRelease::DR3);
        GaiaAsteroidMatcher matcher(&grappa);
        
        // Query a region known to have asteroids
        std::cout << "   Querying Gaia for objects near ecliptic..." << std::endl;
        auto gaia_objects = client.queryCone(180.0, 0.0, 2.0, 18.0);
        
        int asteroid_count = 0;
        for (const auto& obj : gaia_objects) {
            if (matcher.isAsteroid(obj.source_id)) {
                asteroid_count++;
            }
        }
        
        std::cout << "   Gaia objects: " << gaia_objects.size() << std::endl;
        std::cout << "   Identified as asteroids: " << asteroid_count << std::endl;
        
        std::cout << "\n✅ All tests completed!" << std::endl;
        
    } catch (const GaiaException& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
