// Quick test for OpenMP parallelization
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include <iostream>
#include <chrono>
#include <omp.h>

using namespace ioc::gaia;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog.mag18v2>" << std::endl;
        return 1;
    }
    
    std::cout << "OpenMP Configuration:" << std::endl;
    std::cout << "  Max threads: " << omp_get_max_threads() << std::endl;
    std::cout << "  Num procs: " << omp_get_num_procs() << std::endl;
    std::cout << std::endl;
    
    try {
        Mag18CatalogV2 catalog(argv[1]);
        
        double ra = 180.0, dec = 0.0, radius = 5.0;
        
        // Test sequential
        std::cout << "=== Sequential Test ===" << std::endl;
        catalog.setParallelProcessing(false);
        auto start = std::chrono::high_resolution_clock::now();
        auto stars_seq = catalog.queryCone(ra, dec, radius);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_seq = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Time: " << duration_seq.count() << " ms" << std::endl;
        std::cout << "Stars: " << stars_seq.size() << std::endl;
        std::cout << std::endl;
        
        // Test parallel with different thread counts
        for (size_t threads : {2, 4, 8}) {
            std::cout << "=== Parallel Test (" << threads << " threads) ===" << std::endl;
            catalog.setParallelProcessing(true, threads);
            
            start = std::chrono::high_resolution_clock::now();
            auto stars_par = catalog.queryCone(ra, dec, radius);
            end = std::chrono::high_resolution_clock::now();
            auto duration_par = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            std::cout << "Time: " << duration_par.count() << " ms" << std::endl;
            std::cout << "Stars: " << stars_par.size() << std::endl;
            std::cout << "Speedup: " << (double)duration_seq.count() / duration_par.count() << "x" << std::endl;
            std::cout << "Match: " << (stars_par.size() == stars_seq.size() ? "✓" : "✗") << std::endl;
            std::cout << std::endl;
        }
        
        std::cout << "✓ OpenMP tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
