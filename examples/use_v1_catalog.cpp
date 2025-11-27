#include <ioc_gaialib/gaia_mag18_catalog.h>
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog_v1.cat.gz>" << std::endl;
        std::cerr << "Example: " << argv[0] << " ~/catalogs/gaia_mag18.cat.gz" << std::endl;
        return 1;
    }
    
    std::string catalog_path = argv[1];
    
    try {
        // Open V1 catalog (constructor loads it)
        std::cout << "Opening Mag18 V1 catalog: " << catalog_path << std::endl;
        ioc_gaialib::GaiaMag18Catalog catalog(catalog_path);
        
        if (!catalog.isLoaded()) {
            std::cerr << "Failed to load catalog" << std::endl;
            return 1;
        }
        
        auto stats = catalog.getStatistics();
        std::cout << "Catalog loaded successfully!" << std::endl;
        std::cout << "Total stars: " << stats.total_stars << std::endl;
        std::cout << "Magnitude limit: G ≤ " << stats.mag_limit << std::endl;
        std::cout << std::endl;
        
        // Example query 1: Small cone around Sirius (brightest star)
        double ra1 = 101.2875;  // Sirius RA
        double dec1 = -16.7161; // Sirius Dec
        double radius1 = 0.5;   // 0.5 degrees
        
        std::cout << "Query 1: 0.5° cone around Sirius..." << std::endl;
        auto stars1 = catalog.queryCone(ra1, dec1, radius1, 10);
        std::cout << "Found " << stars1.size() << " stars (max 10)" << std::endl;
        
        if (!stars1.empty()) {
            std::cout << std::fixed << std::setprecision(6);
            std::cout << "Brightest star: ID=" << stars1[0].source_id 
                      << " G=" << stars1[0].phot_g_mean_mag << std::endl;
        }
        std::cout << std::endl;
        
        // Example query 2: Bright stars (mag 10-12) in 2° cone
        double ra2 = 180.0;
        double dec2 = 0.0;
        double radius2 = 2.0;
        
        std::cout << "Query 2: Bright stars (G=10-12) in 2° cone at RA=180°, Dec=0°..." << std::endl;
        auto stars2 = catalog.queryConeWithMagnitude(ra2, dec2, radius2, 10.0, 12.0);
        std::cout << "Found " << stars2.size() << " bright stars" << std::endl;
        std::cout << std::endl;
        
        // Print first 5 results
        std::cout << "First 5 results:" << std::endl;
        std::cout << std::string(75, '-') << std::endl;
        
        for (size_t i = 0; i < std::min(stars2.size(), size_t(5)); ++i) {
            const auto& star = stars2[i];
            std::cout << (i+1) << ". ID=" << star.source_id 
                      << " | RA=" << std::setw(10) << star.ra
                      << " | Dec=" << std::setw(10) << star.dec
                      << " | G=" << std::setw(6) << star.phot_g_mean_mag << std::endl;
        }
        std::cout << std::endl;
        
        // Example query 3: Query by source ID
        if (!stars1.empty()) {
            uint64_t source_id = stars1[0].source_id;
            std::cout << "Query 3: Lookup by source ID " << source_id << "..." << std::endl;
            auto star_opt = catalog.queryBySourceId(source_id);
            if (star_opt) {
                std::cout << "Found: RA=" << star_opt->ra 
                          << " Dec=" << star_opt->dec 
                          << " G=" << star_opt->phot_g_mean_mag << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
