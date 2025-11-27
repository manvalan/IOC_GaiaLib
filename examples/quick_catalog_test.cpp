#include "ioc_gaialib/catalog_compiler.h"
#include "ioc_gaialib/gaia_client.h"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace ioc::gaia;

void progressCallback(const CompilationProgress& progress) {
    if (progress.percent >= 0) {
        std::cout << "\r[" << std::setw(3) << progress.percent << "%] "
                  << progress.phase << " | "
                  << "Tile " << progress.current_tile_id << " "
                  << "(" << progress.completed_tiles << "/" << progress.total_tiles << ") | "
                  << progress.stars_processed << " stars";
        
        if (progress.eta_seconds >= 0 && progress.eta_seconds < 3600) {
            int mins = (int)(progress.eta_seconds / 60);
            int secs = (int)(progress.eta_seconds) % 60;
            std::cout << " | ETA: " << mins << "m " << secs << "s";
        }
        
        std::cout << std::string(20, ' ') << std::flush;
        
        if (progress.percent == 100) {
            std::cout << "\n✅ " << progress.message << std::endl;
        }
    } else {
        std::cout << "\n⚠️  " << progress.phase << ": " << progress.message << std::endl;
    }
}

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Quick Gaia DR3 Catalog Test                        ║" << std::endl;
    std::cout << "║  Magnitude limit: 4.0 (bright stars only)           ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════╝\n" << std::endl;
    
    try {
        // Setup
        std::string output_dir = "/tmp/gaia_test_mag4";
        double max_magnitude = 4.0;
        
        std::cout << "📁 Output: " << output_dir << std::endl;
        std::cout << "⭐ Max magnitude: " << max_magnitude << std::endl;
        std::cout << "🔧 HEALPix NSIDE: 32 (12,288 tiles)" << std::endl;
        std::cout << "\nInitializing..." << std::endl;
        
        GaiaClient client(GaiaRelease::DR3);
        GaiaCatalogCompiler compiler(output_dir, 32, GaiaRelease::DR3);
        
        compiler.setClient(&client);
        compiler.setMaxMagnitude(max_magnitude);
        compiler.setEnableCompression(true);
        compiler.setCompressionLevel(6);
        compiler.setLogging("/tmp/gaia_test_mag4.log", true);
        compiler.setParallelWorkers(1);  // Single worker for testing
        
        std::cout << "\n🚀 Starting compilation...\n" << std::endl;
        
        auto start = std::chrono::steady_clock::now();
        auto stats = compiler.compile(progressCallback);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        
        std::cout << "\n\n╔══════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  Compilation Complete                                ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════╝" << std::endl;
        std::cout << "Total stars:       " << stats.total_stars << std::endl;
        std::cout << "Completed tiles:   " << stats.healpix_tiles << " / " << stats.total_tiles << std::endl;
        std::cout << "Coverage:          " << std::fixed << std::setprecision(2) 
                  << stats.coverage_percent << "%" << std::endl;
        std::cout << "Time:              " << duration << " seconds" << std::endl;
        
        double size_mb = stats.disk_size_bytes / (1024.0 * 1024.0);
        std::cout << "Disk size:         " << std::fixed << std::setprecision(2) 
                  << size_mb << " MB" << std::endl;
        std::cout << "\n✅ Log file: /tmp/gaia_test_mag4.log" << std::endl;
        
    } catch (const GaiaException& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
