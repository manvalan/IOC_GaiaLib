#include "ioc_gaialib/catalog_compiler.h"
#include "ioc_gaialib/gaia_client.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <signal.h>

using namespace ioc::gaia;

// Global compiler pointer for signal handling
GaiaCatalogCompiler* g_compiler = nullptr;

void signalHandler(int signum) {
    std::cout << "\n\n🛑 Interrupt received! Saving checkpoint..." << std::endl;
    if (g_compiler) {
        g_compiler->cancel();
    }
}

std::string formatTime(double seconds) {
    if (seconds < 0) return "calculating...";
    
    int hours = (int)(seconds / 3600);
    int mins = (int)((seconds - hours * 3600) / 60);
    int secs = (int)(seconds - hours * 3600 - mins * 60);
    
    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h " << mins << "m " << secs << "s";
    } else if (mins > 0) {
        oss << mins << "m " << secs << "s";
    } else {
        oss << secs << "s";
    }
    return oss.str();
}

void progressCallback(const CompilationProgress& progress) {
    static auto last_update = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    // Update every 500ms
    if (progress.percent >= 0 && progress.percent < 100 &&
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count() < 500) {
        return;
    }
    last_update = now;
    
    if (progress.percent >= 0) {
        std::cout << "\r";
        std::cout << "[" << std::setw(3) << progress.percent << "%] ";
        std::cout << progress.phase << " | ";
        std::cout << "Tile " << progress.current_tile_id << " ";
        std::cout << "(" << progress.completed_tiles << "/" << progress.total_tiles << ") | ";
        std::cout << "RA=" << std::fixed << std::setprecision(1) << progress.current_ra << "° ";
        std::cout << "Dec=" << std::fixed << std::setprecision(1) << progress.current_dec << "° | ";
        std::cout << progress.stars_processed << " stars";
        
        if (progress.download_speed_mbps > 0) {
            std::cout << " | " << std::fixed << std::setprecision(2) 
                     << progress.download_speed_mbps << " MB/s";
        }
        
        if (progress.eta_seconds >= 0) {
            std::cout << " | ETA: " << formatTime(progress.eta_seconds);
        }
        
        std::cout << std::string(20, ' ');  // Clear previous text
        std::cout << std::flush;
        
        if (progress.percent == 100) {
            std::cout << "\n✅ " << progress.message << std::endl;
        }
    } else {
        std::cout << "\n⚠️  " << progress.phase << ": " << progress.message << std::endl;
    }
}

void printStats(const CompilationStats& stats) {
    std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
    std::cout << "║     Catalog Statistics               ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝" << std::endl;
    
    std::cout << "Total stars:       " << std::setw(12) << stats.total_stars << std::endl;
    std::cout << "With cross-match:  " << std::setw(12) << stats.stars_with_crossmatch << std::endl;
    std::cout << "HEALPix tiles:     " << std::setw(12) << stats.healpix_tiles << std::endl;
    std::cout << "Coverage:          " << std::setw(11) << std::fixed 
              << std::setprecision(2) << stats.coverage_percent << "%" << std::endl;
    
    double size_mb = stats.disk_size_bytes / (1024.0 * 1024.0);
    double size_gb = size_mb / 1024.0;
    
    if (size_gb > 1.0) {
        std::cout << "Disk size:         " << std::setw(11) << std::setprecision(2) 
                  << size_gb << " GB" << std::endl;
    } else {
        std::cout << "Disk size:         " << std::setw(11) << std::setprecision(2) 
                  << size_mb << " MB" << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
    std::cout << "║  Gaia DR3 Catalog Compiler          ║" << std::endl;
    std::cout << "║  Resumable full-sky compilation     ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝\n" << std::endl;
    
    // Parse command line
    std::string output_dir = "/tmp/gaia_catalog";
    double max_magnitude = 12.0;
    bool enable_crossmatch = true;
    int workers = 4;
    
    if (argc > 1) output_dir = argv[1];
    if (argc > 2) max_magnitude = std::stod(argv[2]);
    if (argc > 3) enable_crossmatch = (std::string(argv[3]) == "true");
    if (argc > 4) workers = std::stoi(argv[4]);
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Output directory:  " << output_dir << std::endl;
    std::cout << "  Max magnitude:     " << max_magnitude << std::endl;
    std::cout << "  Cross-match:       " << (enable_crossmatch ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Parallel workers:  " << workers << std::endl;
    
    // Estimate disk space
    size_t estimated = GaiaCatalogCompiler::estimateDiskSpace(max_magnitude);
    double estimated_gb = estimated / (1024.0 * 1024.0 * 1024.0);
    std::cout << "  Estimated size:    ~" << std::fixed << std::setprecision(1) 
              << estimated_gb << " GB" << std::endl;
    
    std::cout << "\nPress Ctrl+C to pause and save progress\n" << std::endl;
    
    try {
        // Setup signal handler
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        
        // Create compiler
        GaiaCatalogCompiler compiler(output_dir, 32, GaiaRelease::DR3);
        g_compiler = &compiler;
        
        compiler.setMaxMagnitude(max_magnitude);
        compiler.setEnableCrossMatch(enable_crossmatch);
        compiler.setParallelWorkers(workers);
        
        // Create client
        GaiaClient client(GaiaRelease::DR3);
        client.setRateLimit(10);
        compiler.setClient(&client);
        
        // Check if resuming
        double progress = compiler.getProgress();
        if (progress > 0) {
            std::cout << "📂 Found existing compilation at " << progress << "%" << std::endl;
            std::cout << "   Resuming from checkpoint..." << std::endl;
            
            auto pending = compiler.getPendingTiles();
            std::cout << "   " << pending.size() << " tiles remaining\n" << std::endl;
        } else {
            std::cout << "🆕 Starting new compilation\n" << std::endl;
        }
        
        // Start compilation
        auto start_time = std::chrono::steady_clock::now();
        
        auto stats = compiler.compile(progressCallback);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            end_time - start_time).count();
        
        std::cout << "\n\n";
        
        if (compiler.isComplete()) {
            std::cout << "✅ Compilation COMPLETE in " << duration << " seconds" << std::endl;
            printStats(stats);
            
            std::cout << "Catalog ready at: " << output_dir << std::endl;
            std::cout << "\nYou can now use CompiledCatalog to query offline:\n";
            std::cout << "  CompiledCatalog cat(\"" << output_dir << "\");\n";
            std::cout << "  auto stars = cat.queryRegion(ra, dec, radius);\n";
        } else {
            std::cout << "⏸️  Compilation PAUSED" << std::endl;
            printStats(stats);
            
            std::cout << "To resume, run the same command again:\n";
            std::cout << "  " << argv[0] << " " << output_dir 
                      << " " << max_magnitude << std::endl;
        }
        
        return 0;
        
    } catch (const GaiaException& e) {
        std::cerr << "\n✗ GaiaException: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Exception: " << e.what() << std::endl;
        return 1;
    }
}
