#include "ioc_gaialib/gaia_local_catalog.h"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace ioc_gaialib;

void printStar(const GaiaStar& star) {
    std::cout << "  Source ID: " << star.source_id << "\n";
    std::cout << "  RA/Dec: " << std::fixed << std::setprecision(6)
              << star.ra << "°, " << star.dec << "°\n";
    std::cout << "  Parallax: " << std::setprecision(3) << star.parallax << " ± "
              << star.parallax_error << " mas\n";
    std::cout << "  PM: (" << std::setprecision(2) << star.pmra << ", "
              << star.pmdec << ") mas/yr\n";
    if (star.phot_g_mean_mag > 0) {
        std::cout << "  G mag: " << std::setprecision(3) << star.phot_g_mean_mag << "\n";
    }
    std::cout << "\n";
}

int main(int argc, char** argv) {
    try {
        std::string catalog_path = (argc > 1) ? argv[1] : 
            std::string(getenv("HOME")) + "/catalogs/GRAPPA3E";
        
        std::cout << "═══════════════════════════════════════════════════════\n";
        std::cout << "  Gaia Local Catalog Test (GRAPPA3E)\n";
        std::cout << "═══════════════════════════════════════════════════════\n\n";
        
        std::cout << "Loading catalog from: " << catalog_path << "\n\n";
        
        // Initialize catalog
        auto start = std::chrono::high_resolution_clock::now();
        GaiaLocalCatalog catalog(catalog_path);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Catalog loaded in " << duration.count() << " ms\n\n";
        
        // Get statistics
        auto stats = catalog.getStatistics();
        std::cout << "Catalog statistics:\n";
        std::cout << "  Tiles available: " << stats.tiles_loaded << "\n";
        std::cout << "  (Full Gaia DR3: ~1.8 billion sources)\n\n";
        
        // Test 1: Cone search around Aldebaran
        std::cout << "───────────────────────────────────────────────────────\n";
        std::cout << "Test 1: Cone search around Aldebaran (α Tauri)\n";
        std::cout << "───────────────────────────────────────────────────────\n";
        
        double aldebaran_ra = 68.980163;  // degrees
        double aldebaran_dec = 16.509302; // degrees
        double radius = 0.5;              // degrees
        
        start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCone(aldebaran_ra, aldebaran_dec, radius, 10);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\nSearch: RA=" << aldebaran_ra << "°, Dec=" << aldebaran_dec
                  << "°, radius=" << radius << "°\n";
        std::cout << "Found " << stars.size() << " stars in " << duration.count() << " ms\n\n";
        
        if (!stars.empty()) {
            std::cout << "First 5 results:\n\n";
            for (size_t i = 0; i < std::min(size_t(5), stars.size()); ++i) {
                std::cout << (i+1) << ".\n";
                printStar(stars[i]);
            }
        }
        
        // Test 2: Query by source_id (Sirius)
        std::cout << "───────────────────────────────────────────────────────\n";
        std::cout << "Test 2: Query by source_id (Sirius)\n";
        std::cout << "───────────────────────────────────────────────────────\n";
        
        uint64_t sirius_id = 2947050466531873024ULL;
        
        start = std::chrono::high_resolution_clock::now();
        auto sirius = catalog.queryBySourceId(sirius_id);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\nSearching for source_id: " << sirius_id << "\n";
        std::cout << "Query time: " << duration.count() << " ms\n\n";
        
        if (sirius) {
            std::cout << "Found Sirius:\n\n";
            printStar(*sirius);
        } else {
            std::cout << "⚠ Sirius not found (source_id lookup requires full scan)\n";
            std::cout << "  Note: Building a source_id index would speed this up\n\n";
        }
        
        // Test 3: Pleiades cluster
        std::cout << "───────────────────────────────────────────────────────\n";
        std::cout << "Test 3: Pleiades cluster (M45)\n";
        std::cout << "───────────────────────────────────────────────────────\n";
        
        double pleiades_ra = 56.75;    // degrees
        double pleiades_dec = 24.117;  // degrees
        radius = 1.0;                  // degrees
        
        start = std::chrono::high_resolution_clock::now();
        stars = catalog.queryCone(pleiades_ra, pleiades_dec, radius);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\nSearch: RA=" << pleiades_ra << "°, Dec=" << pleiades_dec
                  << "°, radius=" << radius << "°\n";
        std::cout << "Found " << stars.size() << " stars in " << duration.count() << " ms\n";
        
        // Count by magnitude bins
        int bright = 0, medium = 0, faint = 0;
        for (const auto& star : stars) {
            if (star.phot_g_mean_mag > 0) {
                if (star.phot_g_mean_mag < 10) bright++;
                else if (star.phot_g_mean_mag < 15) medium++;
                else faint++;
            }
        }
        
        std::cout << "\nMagnitude distribution:\n";
        std::cout << "  G < 10:   " << bright << " stars\n";
        std::cout << "  10-15:    " << medium << " stars\n";
        std::cout << "  G > 15:   " << faint << " stars\n\n";
        
        std::cout << "═══════════════════════════════════════════════════════\n";
        std::cout << "✓ All tests completed successfully!\n";
        std::cout << "═══════════════════════════════════════════════════════\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
