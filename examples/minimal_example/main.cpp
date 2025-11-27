#include <ioc_gaialib/gaia_mag18_catalog_v2.h>
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog_path.mag18v2>" << std::endl;
        std::cerr << "Example: " << argv[0] << " gaia_mag18_v2.mag18v2" << std::endl;
        return 1;
    }
    
    std::string catalog_path = argv[1];
    
    try {
        // Open catalog
        std::cout << "Opening catalog: " << catalog_path << std::endl;
        ioc::gaia::Mag18CatalogV2 catalog(catalog_path);
        
        // Enable parallel processing
        catalog.setParallelProcessing(true, 4);
        
        std::cout << "Catalog opened successfully!" << std::endl;
        std::cout << "Total stars: " << catalog.getTotalStars() << std::endl;
        std::cout << "HEALPix NSIDE: " << catalog.getHEALPixNside() << std::endl;
        std::cout << std::endl;
        
        // Example query: 5° cone around M42 (Orion Nebula)
        double ra = 83.8220;   // RA of M42
        double dec = -5.3911;  // Dec of M42
        double radius = 5.0;   // 5 degrees
        
        std::cout << "Querying 5° cone around M42 (RA=" << ra 
                  << ", Dec=" << dec << ")..." << std::endl;
        
        auto stars = catalog.queryCone(ra, dec, radius);
        
        std::cout << "Found " << stars.size() << " stars" << std::endl;
        std::cout << std::endl;
        
        // Print first 10 stars
        std::cout << "First 10 stars:" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        std::cout << std::string(80, '-') << std::endl;
        
        for (size_t i = 0; i < std::min(stars.size(), size_t(10)); ++i) {
            const auto& star = stars[i];
            std::cout << "Star " << (i+1) << ":" << std::endl;
            std::cout << "  Source ID: " << star.source_id << std::endl;
            std::cout << "  RA:        " << std::setw(10) << star.ra << "°" << std::endl;
            std::cout << "  Dec:       " << std::setw(10) << star.dec << "°" << std::endl;
            std::cout << "  G mag:     " << std::setw(8) << star.phot_g_mean_mag << std::endl;
            std::cout << "  Parallax:  " << std::setw(8) << star.parallax << " mas" << std::endl;
            std::cout << std::endl;
        }
        
        // Query brightest stars
        std::cout << "Brightest 5 stars in cone:" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        auto bright = catalog.queryBrightest(ra, dec, radius, 5);
        
        for (size_t i = 0; i < bright.size(); ++i) {
            const auto& star = bright[i];
            std::cout << (i+1) << ". ID=" << star.source_id 
                      << " | G=" << std::setw(6) << star.phot_g_mean_mag
                      << " | RA=" << std::setw(10) << star.ra
                      << " | Dec=" << std::setw(10) << star.dec << std::endl;
        }
        
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "\nMake sure:" << std::endl;
        std::cerr << "  1. The catalog file exists" << std::endl;
        std::cerr << "  2. You have read permissions" << std::endl;
        std::cerr << "  3. The file is a valid .mag18v2 catalog" << std::endl;
        return 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
