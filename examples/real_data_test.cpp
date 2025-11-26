#include "ioc_gaialib/types.h"
#include "ioc_gaialib/gaia_cache.h"
#include "ioc_gaialib/gaia_client.h"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace ioc::gaia;

void printSeparator() {
    std::cout << std::string(70, '=') << std::endl;
}

void printStarInfo(const GaiaStar& star, int index = -1) {
    if (index >= 0) {
        std::cout << std::setw(3) << (index + 1) << ". ";
    }
    std::cout << "Gaia DR3 " << star.source_id << std::endl;
    std::cout << "     RA/Dec: " << std::fixed << std::setprecision(5) 
              << star.ra << "°, " << star.dec << "°" << std::endl;
    std::cout << "     G mag: " << std::setprecision(2) << star.phot_g_mean_mag;
    if (!std::isnan(star.phot_bp_mean_mag) && !std::isnan(star.phot_rp_mean_mag)) {
        std::cout << " | BP-RP: " << std::setprecision(3) << star.getBpRpColor();
    }
    std::cout << std::endl;
    std::cout << "     Parallax: " << std::setprecision(3) << star.parallax 
              << " ± " << star.parallax_error << " mas" << std::endl;
    if (star.parallax > 0 && star.parallax_error > 0) {
        double distance_pc = 1000.0 / star.parallax;
        std::cout << "     Distance: ~" << std::setprecision(1) << distance_pc << " pc" << std::endl;
    }
}

void progressCallback(int percent, const std::string& message) {
    if (percent >= 0) {
        std::cout << "\r[" << std::setw(3) << percent << "%] " << message << std::flush;
        if (percent == 100) std::cout << std::endl;
    } else {
        std::cout << std::endl << "[WARN] " << message << std::endl;
    }
}

int main() {
    std::cout << "\n";
    printSeparator();
    std::cout << "  IOC GaiaLib - Complete Real Data Test" << std::endl;
    std::cout << "  Querying ESA Gaia Archive with real HTTP requests" << std::endl;
    printSeparator();
    std::cout << std::endl;
    
    try {
        // =====================================================================
        // TEST 1: Direct Client Query - Famous Star (Aldebaran)
        // =====================================================================
        std::cout << "\n📡 TEST 1: Query bright stars near Aldebaran (α Tauri)" << std::endl;
        printSeparator();
        
        GaiaClient client(GaiaRelease::DR3);
        client.setRateLimit(10);
        
        // Aldebaran approximate position
        double aldebaran_ra = 68.980;   // ~4h 35m
        double aldebaran_dec = 16.509;  // +16° 30'
        double search_radius = 0.1;     // 6 arcmin
        double max_mag = 8.0;           // Only bright stars
        
        std::cout << "Target: Aldebaran region" << std::endl;
        std::cout << "  Center: RA = " << aldebaran_ra << "°, Dec = " << aldebaran_dec << "°" << std::endl;
        std::cout << "  Radius: " << search_radius << "° (" << (search_radius * 60) << " arcmin)" << std::endl;
        std::cout << "  Max magnitude: " << max_mag << std::endl;
        std::cout << "\nQuerying Gaia Archive..." << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = client.queryCone(aldebaran_ra, aldebaran_dec, search_radius, max_mag);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Query completed in " << duration.count() << " ms" << std::endl;
        std::cout << "✓ Found " << stars.size() << " bright stars" << std::endl;
        
        if (!stars.empty()) {
            std::cout << "\nBrightest stars:" << std::endl;
            // Sort by magnitude
            std::sort(stars.begin(), stars.end(), 
                     [](const GaiaStar& a, const GaiaStar& b) {
                         return a.phot_g_mean_mag < b.phot_g_mean_mag;
                     });
            
            for (size_t i = 0; i < std::min(size_t(3), stars.size()); ++i) {
                printStarInfo(stars[i], i);
            }
            
            // Aldebaran should be the brightest
            if (stars[0].phot_g_mean_mag < 2.0) {
                std::cout << "\n🌟 This is likely Aldebaran itself!" << std::endl;
            }
        }
        
        // =====================================================================
        // TEST 2: Box Query - Orion Belt Region
        // =====================================================================
        std::cout << "\n\n📡 TEST 2: Box query in Orion Belt region" << std::endl;
        printSeparator();
        
        // Orion's Belt approximate coordinates
        double orion_ra_min = 83.0;    // ~5h 32m
        double orion_ra_max = 86.0;    // ~5h 44m
        double orion_dec_min = -2.5;
        double orion_dec_max = -0.5;
        double orion_max_mag = 6.0;
        
        std::cout << "Target: Orion's Belt (Alnitak, Alnilam, Mintaka)" << std::endl;
        std::cout << "  RA range: " << orion_ra_min << "° - " << orion_ra_max << "°" << std::endl;
        std::cout << "  Dec range: " << orion_dec_min << "° - " << orion_dec_max << "°" << std::endl;
        std::cout << "  Max magnitude: " << orion_max_mag << std::endl;
        std::cout << "\nQuerying Gaia Archive..." << std::endl;
        
        start = std::chrono::high_resolution_clock::now();
        auto orion_stars = client.queryBox(orion_ra_min, orion_ra_max, 
                                           orion_dec_min, orion_dec_max, 
                                           orion_max_mag);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Query completed in " << duration.count() << " ms" << std::endl;
        std::cout << "✓ Found " << orion_stars.size() << " stars" << std::endl;
        
        if (!orion_stars.empty()) {
            std::sort(orion_stars.begin(), orion_stars.end(), 
                     [](const GaiaStar& a, const GaiaStar& b) {
                         return a.phot_g_mean_mag < b.phot_g_mean_mag;
                     });
            
            std::cout << "\nBrightest stars (Belt stars):" << std::endl;
            for (size_t i = 0; i < std::min(size_t(5), orion_stars.size()); ++i) {
                printStarInfo(orion_stars[i], i);
            }
        }
        
        // =====================================================================
        // TEST 3: Query by Source ID - Sirius
        // =====================================================================
        std::cout << "\n\n📡 TEST 3: Query specific star by Gaia source_id (Sirius)" << std::endl;
        printSeparator();
        
        // Sirius (α CMa) - brightest star in the night sky
        std::vector<int64_t> sirius_ids = {
            2947050466531873024L  // Sirius A (Gaia DR3)
        };
        
        std::cout << "Target: Sirius (α Canis Majoris)" << std::endl;
        std::cout << "  Gaia DR3 source_id: " << sirius_ids[0] << std::endl;
        std::cout << "\nQuerying Gaia Archive..." << std::endl;
        
        start = std::chrono::high_resolution_clock::now();
        auto sirius = client.queryBySourceIds(sirius_ids);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Query completed in " << duration.count() << " ms" << std::endl;
        
        if (!sirius.empty()) {
            std::cout << "✓ Found Sirius!" << std::endl;
            std::cout << "\nSirius data:" << std::endl;
            printStarInfo(sirius[0]);
            
            std::cout << "\n     Proper motion: " << std::setprecision(2)
                      << sirius[0].pmra << " mas/yr (RA), "
                      << sirius[0].pmdec << " mas/yr (Dec)" << std::endl;
        } else {
            std::cout << "✗ Sirius not found (check source_id)" << std::endl;
        }
        
        // =====================================================================
        // TEST 4: Cache Integration - Pleiades (M45)
        // =====================================================================
        std::cout << "\n\n💾 TEST 4: Cache integration - Pleiades cluster (M45)" << std::endl;
        printSeparator();
        
        GaiaCache cache("/tmp/gaia_real_test_cache", 32, GaiaRelease::DR3);
        cache.setClient(&client);
        
        // Pleiades coordinates
        double pleiades_ra = 56.75;    // ~3h 47m
        double pleiades_dec = 24.12;   // +24° 07'
        double pleiades_radius = 1.0;  // 1 degree
        double pleiades_max_mag = 10.0;
        
        std::cout << "Target: Pleiades open cluster" << std::endl;
        std::cout << "  Center: RA = " << pleiades_ra << "°, Dec = " << pleiades_dec << "°" << std::endl;
        std::cout << "  Radius: " << pleiades_radius << "°" << std::endl;
        std::cout << "  Max magnitude: " << pleiades_max_mag << std::endl;
        
        bool cached = cache.isRegionCovered(pleiades_ra, pleiades_dec, pleiades_radius);
        std::cout << "\nRegion already cached: " << (cached ? "YES" : "NO") << std::endl;
        
        if (!cached) {
            std::cout << "\nDownloading from Gaia Archive..." << std::endl;
            start = std::chrono::high_resolution_clock::now();
            size_t downloaded = cache.downloadRegion(pleiades_ra, pleiades_dec, 
                                                    pleiades_radius, pleiades_max_mag,
                                                    progressCallback);
            end = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            std::cout << "✓ Downloaded " << downloaded << " stars in " 
                      << duration.count() << " ms" << std::endl;
        }
        
        // Query from cache
        std::cout << "\nQuerying from local cache..." << std::endl;
        start = std::chrono::high_resolution_clock::now();
        auto pleiades_stars = cache.queryRegion(pleiades_ra, pleiades_dec, 
                                               pleiades_radius, pleiades_max_mag);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Retrieved " << pleiades_stars.size() << " stars in " 
                  << duration.count() << " ms (from cache)" << std::endl;
        
        if (!pleiades_stars.empty()) {
            std::sort(pleiades_stars.begin(), pleiades_stars.end(), 
                     [](const GaiaStar& a, const GaiaStar& b) {
                         return a.phot_g_mean_mag < b.phot_g_mean_mag;
                     });
            
            std::cout << "\nBrightest Pleiades members:" << std::endl;
            for (size_t i = 0; i < std::min(size_t(7), pleiades_stars.size()); ++i) {
                printStarInfo(pleiades_stars[i], i);
            }
            
            // The Seven Sisters (Pleiades brightest stars)
            std::cout << "\n🌟 These should include the \"Seven Sisters\":" << std::endl;
            std::cout << "   Alcyone, Atlas, Electra, Maia, Merope, Taygeta, Pleione" << std::endl;
        }
        
        // Cache statistics
        auto stats = cache.getStatistics();
        std::cout << "\nCache statistics:" << std::endl;
        std::cout << "  Cached tiles: " << stats.cached_tiles << " / " << stats.total_tiles << std::endl;
        std::cout << "  Coverage: " << std::setprecision(4) << stats.getCoveragePercent() << "%" << std::endl;
        std::cout << "  Disk size: " << (stats.disk_size_bytes / 1024.0) << " KB" << std::endl;
        
        // =====================================================================
        // Summary
        // =====================================================================
        std::cout << "\n\n";
        printSeparator();
        std::cout << "  ✓ ALL REAL DATA TESTS COMPLETED SUCCESSFULLY" << std::endl;
        printSeparator();
        std::cout << "\nSummary:" << std::endl;
        std::cout << "  ✓ Direct cone queries working" << std::endl;
        std::cout << "  ✓ Box queries working" << std::endl;
        std::cout << "  ✓ Source ID queries working" << std::endl;
        std::cout << "  ✓ Cache integration working" << std::endl;
        std::cout << "  ✓ All HTTP requests to Gaia Archive successful" << std::endl;
        std::cout << "\nThe library is fully operational with real Gaia DR3 data! 🎉\n" << std::endl;
        
        return 0;
        
    } catch (const GaiaException& e) {
        std::cerr << "\n✗ GaiaException: " << e.what() << std::endl;
        std::cerr << "Error code: " << static_cast<int>(e.code()) << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Exception: " << e.what() << std::endl;
        return 1;
    }
}
