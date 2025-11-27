// Test parallel processing of Mag18 V2 catalog
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <random>

using namespace ioc::gaia;

// Benchmark cone search with different thread counts
void benchmarkParallelConeSearch(const std::string& catalog_path) {
    std::cout << "\n=== PARALLEL CONE SEARCH BENCHMARK ===" << std::endl;
    
    // Test positions (random RA/Dec)
    std::vector<std::pair<double, double>> test_positions = {
        {45.0, 30.0},
        {120.5, -15.3},
        {200.8, 45.2},
        {315.3, -60.5},
        {90.0, 0.0}
    };
    
    double radius = 5.0; // 5 degree cone
    
    // Test with different thread configurations
    std::vector<size_t> thread_counts = {1, 2, 4, 8};
    
    for (size_t num_threads : thread_counts) {
        Mag18CatalogV2 catalog(catalog_path);
        catalog.setParallelProcessing(true, num_threads);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        size_t total_stars = 0;
        for (const auto& pos : test_positions) {
            auto stars = catalog.queryCone(pos.first, pos.second, radius);
            total_stars += stars.size();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Threads: " << num_threads 
                  << " | Time: " << duration.count() << " ms"
                  << " | Stars: " << total_stars << std::endl;
    }
}

// Test concurrent queries from multiple threads
void testConcurrentQueries(const std::string& catalog_path) {
    std::cout << "\n=== CONCURRENT QUERY TEST ===" << std::endl;
    
    Mag18CatalogV2 catalog(catalog_path);
    catalog.setParallelProcessing(true, 4);
    
    const size_t num_query_threads = 8;
    std::vector<std::thread> threads;
    std::atomic<size_t> total_queries(0);
    std::atomic<size_t> total_stars(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Launch multiple threads doing random queries
    for (size_t i = 0; i < num_query_threads; ++i) {
        threads.emplace_back([&catalog, &total_queries, &total_stars, i]() {
            std::mt19937 rng(i * 12345);
            std::uniform_real_distribution<double> ra_dist(0.0, 360.0);
            std::uniform_real_distribution<double> dec_dist(-90.0, 90.0);
            
            for (size_t q = 0; q < 10; ++q) {
                double ra = ra_dist(rng);
                double dec = dec_dist(rng);
                auto stars = catalog.queryCone(ra, dec, 1.0); // 1 degree cone
                
                total_queries.fetch_add(1);
                total_stars.fetch_add(stars.size());
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Concurrent threads: " << num_query_threads << std::endl;
    std::cout << "Total queries: " << total_queries.load() << std::endl;
    std::cout << "Total stars found: " << total_stars.load() << std::endl;
    std::cout << "Total time: " << duration.count() << " ms" << std::endl;
    std::cout << "Avg time/query: " << (duration.count() / (double)total_queries.load()) 
              << " ms" << std::endl;
}

// Test thread-safety with magnitude queries
void testMagnitudeQueryThreadSafety(const std::string& catalog_path) {
    std::cout << "\n=== MAGNITUDE QUERY THREAD-SAFETY TEST ===" << std::endl;
    
    Mag18CatalogV2 catalog(catalog_path);
    catalog.setParallelProcessing(true, 4);
    
    const size_t num_threads = 4;
    std::vector<std::thread> threads;
    std::atomic<bool> all_ok(true);
    
    // Launch threads doing magnitude queries
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([&catalog, &all_ok, i]() {
            try {
                // Query same region with different magnitude ranges
                double ra = 180.0;
                double dec = 0.0;
                double radius = 5.0;
                
                for (size_t q = 0; q < 20; ++q) {
                    double mag_min = 10.0 + (i * 2.0);
                    double mag_max = mag_min + 2.0;
                    
                    auto stars = catalog.queryConeWithMagnitude(ra, dec, radius, 
                                                                 mag_min, mag_max);
                    
                    // Verify all stars are in magnitude range
                    for (const auto& star : stars) {
                        if (star.phot_g_mean_mag < mag_min || star.phot_g_mean_mag > mag_max) {
                            std::cerr << "Thread " << i << ": Magnitude out of range!" << std::endl;
                            all_ok.store(false);
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Thread " << i << " exception: " << e.what() << std::endl;
                all_ok.store(false);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    if (all_ok.load()) {
        std::cout << "✓ All magnitude queries thread-safe and correct" << std::endl;
    } else {
        std::cout << "✗ Thread-safety issues detected!" << std::endl;
    }
}

// Performance comparison: sequential vs parallel
void compareSequentialVsParallel(const std::string& catalog_path) {
    std::cout << "\n=== SEQUENTIAL VS PARALLEL COMPARISON ===" << std::endl;
    
    double ra = 180.0, dec = 0.0, radius = 10.0; // Large query
    
    // Sequential
    {
        Mag18CatalogV2 catalog(catalog_path);
        catalog.setParallelProcessing(false);
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCone(ra, dec, radius);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Sequential: " << duration.count() << " ms | Stars: " << stars.size() << std::endl;
    }
    
    // Parallel
    {
        Mag18CatalogV2 catalog(catalog_path);
        catalog.setParallelProcessing(true, 4);
        
        auto start = std::chrono::high_resolution_clock::now();
        auto stars = catalog.queryCone(ra, dec, radius);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Parallel (4 threads): " << duration.count() << " ms | Stars: " << stars.size() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog_path.mag18v2>" << std::endl;
        return 1;
    }
    
    std::string catalog_path = argv[1];
    
    std::cout << "Testing Mag18 V2 Parallel Processing" << std::endl;
    std::cout << "Catalog: " << catalog_path << std::endl;
    std::cout << "Hardware threads: " << std::thread::hardware_concurrency() << std::endl;
    
    try {
        // Run tests
        compareSequentialVsParallel(catalog_path);
        benchmarkParallelConeSearch(catalog_path);
        testConcurrentQueries(catalog_path);
        testMagnitudeQueryThreadSafety(catalog_path);
        
        std::cout << "\n✓ All parallel processing tests completed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
