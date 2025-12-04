/**
 * Test queryCone con parametri usati da IOoccultCalc
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <ioc_gaialib/unified_gaia_catalog.h>

int main() {
    std::cout << "=== Test queryCone per IOccultCalc ===" << std::endl;
    
    // Inizializza catalogo
    std::string home = getenv("HOME");
    std::string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
    })";
    
    if (!ioc::gaia::UnifiedGaiaCatalog::initialize(config)) {
        std::cerr << "Failed to initialize catalog!" << std::endl;
        return 1;
    }
    
    auto& catalog = ioc::gaia::UnifiedGaiaCatalog::getInstance();
    auto info = catalog.getCatalogInfo();
    
    std::cout << "Catalog ready with " << info.total_stars << " stars" << std::endl;
    
    // Test 1: Query con raggio 7° (come IOccultCalc)
    std::cout << "\n=== Test 1: Raggio 7° (parametri IOccultCalc) ===" << std::endl;
    {
        ioc::gaia::QueryParams params;
        params.ra_center = 83.0;      // Orione
        params.dec_center = -5.0;
        params.radius = 7.0;          // 7° come IOccultCalc
        params.max_magnitude = 18.0;  // Default mag limit
        
        std::cout << "Query: RA=" << params.ra_center 
                  << "° Dec=" << params.dec_center 
                  << "° Radius=" << params.radius << "°" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCone(params);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Trovate " << stars.size() << " stelle in " << duration.count() << " ms" << std::endl;
        
        if (stars.size() > 0) {
            std::cout << "\nPrime 5 stelle:" << std::endl;
            for (size_t i = 0; i < std::min(stars.size(), size_t(5)); i++) {
                const auto& s = stars[i];
                std::cout << "  " << s.source_id
                          << " RA=" << std::fixed << std::setprecision(4) << s.ra
                          << " Dec=" << s.dec
                          << " G=" << std::setprecision(2) << s.phot_g_mean_mag
                          << std::endl;
            }
            
            // Verifica che le stelle siano effettivamente nel raggio
            std::cout << "\nVerifica distanze dalle prime 5 stelle:" << std::endl;
            for (size_t i = 0; i < std::min(stars.size(), size_t(5)); i++) {
                const auto& s = stars[i];
                
                // Calcola distanza angolare approssimativa
                double dra = (s.ra - params.ra_center) * cos(params.dec_center * M_PI / 180.0);
                double ddec = s.dec - params.dec_center;
                double dist = sqrt(dra * dra + ddec * ddec);
                
                std::cout << "  Stella " << i+1 << ": distanza = " 
                          << std::fixed << std::setprecision(4) << dist << "°";
                if (dist > params.radius) {
                    std::cout << " ❌ FUORI RAGGIO!";
                }
                std::cout << std::endl;
            }
        }
    }
    
    // Test 2: Query con raggio piccolo per confronto
    std::cout << "\n=== Test 2: Raggio 1° (per confronto) ===" << std::endl;
    {
        ioc::gaia::QueryParams params;
        params.ra_center = 83.0;
        params.dec_center = -5.0;
        params.radius = 1.0;          // 1° di raggio
        params.max_magnitude = 18.0;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCone(params);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Trovate " << stars.size() << " stelle in " << duration.count() << " ms" << std::endl;
    }
    
    // Test 3: Query con magnitudine filtrata
    std::cout << "\n=== Test 3: Raggio 7°, mag < 12 ===" << std::endl;
    {
        ioc::gaia::QueryParams params;
        params.ra_center = 83.0;
        params.dec_center = -5.0;
        params.radius = 7.0;
        params.max_magnitude = 12.0;  // Solo stelle luminose
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCone(params);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Trovate " << stars.size() << " stelle in " << duration.count() << " ms" << std::endl;
    }
    
    // Test 4: Verifica una posizione specifica (come un asteroide)
    std::cout << "\n=== Test 4: Posizione specifica ===" << std::endl;
    {
        ioc::gaia::QueryParams params;
        params.ra_center = 180.5;     // Posizione arbitraria
        params.dec_center = 10.3;
        params.radius = 7.0;
        params.max_magnitude = 18.0;
        
        std::cout << "Query: RA=" << params.ra_center 
                  << "° Dec=" << params.dec_center 
                  << "° Radius=" << params.radius << "°" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCone(params);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Trovate " << stars.size() << " stelle in " << duration.count() << " ms" << std::endl;
        
        if (stars.size() > 0) {
            // Verifica alcune stelle casuali
            std::cout << "\nControllo distanze da 5 stelle casuali:" << std::endl;
            for (size_t i = 0; i < std::min(stars.size(), size_t(5)); i++) {
                size_t idx = (i * stars.size()) / 5;  // Sample distribuito
                const auto& s = stars[idx];
                
                double dra = (s.ra - params.ra_center) * cos(params.dec_center * M_PI / 180.0);
                double ddec = s.dec - params.dec_center;
                double dist = sqrt(dra * dra + ddec * ddec);
                
                std::cout << "  Stella #" << idx << ": RA=" << s.ra 
                          << " Dec=" << s.dec
                          << " → distanza=" << std::fixed << std::setprecision(4) << dist << "°";
                if (dist > params.radius) {
                    std::cout << " ❌ ERRORE: fuori raggio!";
                } else {
                    std::cout << " ✓";
                }
                std::cout << std::endl;
            }
        }
    }
    
    std::cout << "\n=== Test completati ===" << std::endl;
    
    ioc::gaia::UnifiedGaiaCatalog::shutdown();
    return 0;
}
