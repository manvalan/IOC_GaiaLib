#include "ioc_gaialib/gaia_local_catalog.h"
#include <iostream>
#include <iomanip>

using namespace ioc_gaialib;

int main() {
    std::cout << "=== TEST GRAPPA3E (Single Tile) ===" << std::endl;
    
    try {
        // Load catalog
        GaiaLocalCatalog catalog(std::string(getenv("HOME")) + "/catalogs/GRAPPA3E");
        
        if (!catalog.isLoaded()) {
            std::cerr << "Failed to load catalog!" << std::endl;
            return 1;
        }
        
        std::cout << "✅ Catalog loaded successfully" << std::endl << std::endl;
        
        // Test small cone search around RA=1°, Dec=10.5°
        std::cout << "🔍 Cone search: RA=1.5°, Dec=10.5°, radius=0.5°" << std::endl;
        auto stars = catalog.queryCone(1.5, 10.5, 0.5, 20);  // Max 20 results
        
        std::cout << "Found " << stars.size() << " stars:" << std::endl << std::endl;
        
        for (size_t i = 0; i < std::min(stars.size(), size_t(10)); ++i) {
            const auto& star = stars[i];
            std::cout << std::fixed << std::setprecision(4);
            std::cout << (i+1) << ". ID=" << star.source_id 
                      << " RA=" << star.ra << "° Dec=" << star.dec << "°";
            
            if (star.phot_g_mean_mag > 0) {
                std::cout << " G=" << std::setprecision(2) << star.phot_g_mean_mag;
            }
            std::cout << std::endl;
        }
        
        if (stars.size() > 10) {
            std::cout << "... (" << (stars.size() - 10) << " more)" << std::endl;
        }
        
        std::cout << std::endl << "✅ Test completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
