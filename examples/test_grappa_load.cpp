#include "ioc_gaialib/gaia_local_catalog.h"
#include <iostream>

int main() {
    std::string grappa_path = "/Users/michelebigi/catalogs/GRAPPA3E";
    
    std::cout << "Testing GRAPPA3E catalog loading...\n";
    std::cout << "Path: " << grappa_path << "\n\n";
    
    ioc_gaialib::GaiaLocalCatalog catalog(grappa_path);
    
    if (!catalog.isLoaded()) {
        std::cerr << "❌ Failed to load catalog\n";
        return 1;
    }
    
    std::cout << "✅ Catalog loaded\n\n";
    
    // Test query: Sirius region (should have many bright stars)
    double ra = 101.287;   // Sirius RA
    double dec = -16.716;  // Sirius Dec
    double radius = 5.0;   // 5 degree radius
    
    std::cout << "Querying 5° cone around Sirius (RA=" << ra << ", Dec=" << dec << ")...\n";
    
    auto stars = catalog.queryCone(ra, dec, radius, 100);  // Max 100 stars
    
    std::cout << "Found " << stars.size() << " stars\n\n";
    
    if (stars.size() > 0) {
        std::cout << "First 10 stars:\n";
        for (size_t i = 0; i < std::min(size_t(10), stars.size()); ++i) {
            const auto& star = stars[i];
            std::cout << "  ID=" << star.source_id 
                      << " RA=" << star.ra 
                      << " Dec=" << star.dec
                      << " G=" << star.phot_g_mean_mag << "\n";
        }
    }
    
    // Test with magnitude filter: G ≤ 18
    std::cout << "\nQuerying stars with G ≤ 18...\n";
    auto bright_stars = catalog.queryConeWithMagnitude(ra, dec, radius, -5.0, 18.0, 100);
    
    std::cout << "Found " << bright_stars.size() << " stars with G ≤ 18\n";
    
    if (bright_stars.size() > 0) {
        // Find brightest
        auto brightest = *std::min_element(bright_stars.begin(), bright_stars.end(),
            [](const ioc_gaialib::GaiaStar& a, const ioc_gaialib::GaiaStar& b) {
                return a.phot_g_mean_mag < b.phot_g_mean_mag;
            });
        std::cout << "Brightest: ID=" << brightest.source_id 
                  << " G=" << brightest.phot_g_mean_mag << "\n";
    }
    
    return 0;
}
