#include "ioc_gaialib/types.h"
#include "ioc_gaialib/gaia_cache.h"
#include "ioc_gaialib/gaia_client.h"
#include <iostream>
#include <iomanip>

using namespace ioc::gaia;

void progressCallback(int percent, const std::string& message) {
    if (percent >= 0) {
        std::cout << "[" << std::setw(3) << percent << "%] " << message << std::endl;
    } else {
        std::cout << "[WARN] " << message << std::endl;
    }
}

int main() {
    std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
    std::cout << "║  IOC GaiaLib - Integration Test     ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝\n" << std::endl;
    
    try {
        // Create client
        std::cout << "=== Creating Gaia Client ===" << std::endl;
        GaiaClient client(GaiaRelease::DR3);
        client.setRateLimit(10);  // 10 queries per minute
        std::cout << "Client URL: " << client.getTapUrl() << std::endl;
        std::cout << "Table: " << client.getTableName() << "\n" << std::endl;
        
        // Create cache
        std::cout << "=== Creating Cache ===" << std::endl;
        GaiaCache cache("/tmp/gaia_integration_test", 32, GaiaRelease::DR3);
        cache.setClient(&client);
        std::cout << "Cache dir: " << cache.getCacheDir() << std::endl;
        std::cout << "HEALPix NSIDE: " << cache.getNside() << "\n" << std::endl;
        
        // Test coordinates: Aldebaran region (Taurus)
        double ra = 68.98;   // Aldebaran RA
        double dec = 16.51;  // Aldebaran Dec
        double radius = 0.5; // 0.5 degree radius
        double max_mag = 12.0;
        
        std::cout << "=== Target Region ===" << std::endl;
        std::cout << "Center: RA=" << ra << "°, Dec=" << dec << "°" << std::endl;
        std::cout << "Radius: " << radius << "°" << std::endl;
        std::cout << "Max magnitude: " << max_mag << "\n" << std::endl;
        
        // Check if already cached
        bool is_cached = cache.isRegionCovered(ra, dec, radius);
        std::cout << "Region cached: " << (is_cached ? "YES" : "NO") << std::endl;
        
        if (!is_cached) {
            std::cout << "\n=== Downloading Region ===" << std::endl;
            std::cout << "NOTE: This will make a real HTTP request to Gaia Archive" << std::endl;
            std::cout << "Press Ctrl+C to cancel or wait..." << std::endl;
            
            try {
                size_t num_stars = cache.downloadRegion(ra, dec, radius, max_mag, progressCallback);
                std::cout << "\n✓ Downloaded " << num_stars << " stars" << std::endl;
            } catch (const GaiaException& e) {
                std::cout << "\n⚠ Download failed (expected if no network): " << e.what() << std::endl;
                std::cout << "This is normal for testing - continuing with empty cache...\n" << std::endl;
            }
        }
        
        // Query from cache
        std::cout << "\n=== Querying Cache ===" << std::endl;
        try {
            auto stars = cache.queryRegion(ra, dec, radius, max_mag);
            std::cout << "Found " << stars.size() << " stars in cache" << std::endl;
            
            if (!stars.empty()) {
                std::cout << "\nFirst 5 stars:" << std::endl;
                for (size_t i = 0; i < std::min(size_t(5), stars.size()); ++i) {
                    const auto& s = stars[i];
                    std::cout << "  " << s.getDesignation() 
                              << " | RA=" << std::fixed << std::setprecision(4) << s.ra
                              << "° Dec=" << s.dec 
                              << "° | G=" << std::setprecision(2) << s.phot_g_mean_mag << std::endl;
                }
            }
        } catch (const GaiaException& e) {
            std::cout << "Query failed: " << e.what() << std::endl;
            std::cout << "(This is expected if region was not downloaded)" << std::endl;
        }
        
        // Cache statistics
        std::cout << "\n=== Cache Statistics ===" << std::endl;
        auto stats = cache.getStatistics();
        std::cout << "Total tiles: " << stats.total_tiles << std::endl;
        std::cout << "Cached tiles: " << stats.cached_tiles << std::endl;
        std::cout << "Coverage: " << std::fixed << std::setprecision(2) 
                  << stats.getCoveragePercent() << "%" << std::endl;
        std::cout << "Disk size: " << (stats.disk_size_bytes / 1024.0) << " KB" << std::endl;
        
        // Direct client query (without cache)
        std::cout << "\n=== Direct Client Query Test ===" << std::endl;
        std::cout << "Attempting small direct query (1 arcmin, mag<8)..." << std::endl;
        try {
            auto direct_stars = client.queryCone(ra, dec, 1.0/60.0, 8.0);
            std::cout << "✓ Found " << direct_stars.size() << " bright stars" << std::endl;
            
            if (!direct_stars.empty()) {
                std::cout << "Brightest star: " << direct_stars[0].getDesignation()
                          << " (G=" << std::setprecision(2) << direct_stars[0].phot_g_mean_mag << ")" << std::endl;
            }
        } catch (const GaiaException& e) {
            std::cout << "✗ Direct query failed: " << e.what() << std::endl;
            std::cout << "(Check network connection to gea.esac.esa.int)" << std::endl;
        }
        
        std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
        std::cout << "║  ✓ INTEGRATION TEST COMPLETE        ║" << std::endl;
        std::cout << "╚══════════════════════════════════════╝\n" << std::endl;
        
        return 0;
        
    } catch (const GaiaException& e) {
        std::cerr << "\n✗ GaiaException: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Exception: " << e.what() << std::endl;
        return 1;
    }
}
