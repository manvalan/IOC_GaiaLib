#include "ioc_gaialib/gaia_client.h"
#include <iostream>
#include <iomanip>

using namespace ioc::gaia;

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Gaia DR3 Magnitude 17 Sample Test                  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════╝\n" << std::endl;
    
    try {
        GaiaClient client(GaiaRelease::DR3);
        
        // Test diverse regioni del cielo
        struct TestRegion {
            std::string name;
            double ra, dec, radius;
        };
        
        std::vector<TestRegion> regions = {
            {"Polo Nord Celeste", 0.0, 89.0, 2.0},
            {"Equatore (0h)", 0.0, 0.0, 2.0},
            {"Equatore (6h)", 90.0, 0.0, 2.0},
            {"Equatore (12h)", 180.0, 0.0, 2.0},
            {"Equatore (18h)", 270.0, 0.0, 2.0},
            {"Piano Galattico", 180.0, -60.0, 2.0},
            {"Polo Sud Celeste", 0.0, -89.0, 2.0}
        };
        
        size_t total_stars = 0;
        double max_mag = 17.0;
        
        std::cout << "🔍 Campionando 7 regioni del cielo (raggio 2°, mag ≤ 17)...\n" << std::endl;
        
        for (const auto& region : regions) {
            std::cout << "📍 " << std::setw(25) << std::left << region.name 
                      << " (RA=" << std::fixed << std::setprecision(1) 
                      << region.ra << "°, Dec=" << region.dec << "°) ... " << std::flush;
            
            try {
                auto stars = client.queryCone(region.ra, region.dec, region.radius, max_mag);
                total_stars += stars.size();
                
                std::cout << stars.size() << " stelle";
                
                if (!stars.empty()) {
                    // Trova mag min e max
                    double min_mag = 99.0, max_mag_found = 0.0;
                    for (const auto& star : stars) {
                        if (star.phot_g_mean_mag < min_mag) min_mag = star.phot_g_mean_mag;
                        if (star.phot_g_mean_mag > max_mag_found) max_mag_found = star.phot_g_mean_mag;
                    }
                    std::cout << " (mag " << std::fixed << std::setprecision(1) 
                             << min_mag << "-" << max_mag_found << ")";
                }
                std::cout << std::endl;
                
            } catch (const GaiaException& e) {
                std::cout << "❌ Error: " << e.what() << std::endl;
            }
        }
        
        std::cout << "\n╔══════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  Risultati                                           ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════╝" << std::endl;
        std::cout << "Regioni campionate: " << regions.size() << std::endl;
        std::cout << "Stelle totali:      " << total_stars << std::endl;
        std::cout << "Media per regione:  " << (total_stars / regions.size()) << " stelle" << std::endl;
        
        // Stima per tutto il cielo
        // Cielo totale: ~41,253 gradi quadrati
        // Ogni regione: π * 2² ≈ 12.57 gradi quadrati
        // 7 regioni ≈ 88 gradi quadrati
        double sampled_area = 7 * 3.14159 * 2.0 * 2.0;
        double total_sky = 41253.0;
        size_t estimated_total = (size_t)((double)total_stars * total_sky / sampled_area);
        
        std::cout << "\n📊 Stima per l'intero cielo:" << std::endl;
        std::cout << "   Area campionata:  " << std::fixed << std::setprecision(0) 
                  << sampled_area << " deg²" << std::endl;
        std::cout << "   Stelle stimate:   ~" << (estimated_total / 1000000) << " milioni" << std::endl;
        
        std::cout << "\n💡 Note:" << std::endl;
        std::cout << "   - Magnitudine 17: stelle visibili con piccoli telescopi" << std::endl;
        std::cout << "   - Gaia DR3 completo: ~1.8 miliardi di stelle (mag ≤ 21)" << std::endl;
        std::cout << "   - Per catalog completo mag ≤ 17: tempo stimato ~20-30 ore" << std::endl;
        
    } catch (const GaiaException& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
