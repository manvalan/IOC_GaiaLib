#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include "../include/ioc_gaialib/multifile_catalog_v2.h"
#include "../include/ioc_gaialib/concurrent_multifile_catalog_v2.h"

using namespace ioc::gaia;
using namespace std;

void testOriginalCatalog(const string& catalog_path, int num_threads, int queries_per_thread) {
    cout << "\n🔄 Testing Original MultiFile Catalog\n";
    cout << "═══════════════════════════════════════\n";
    
    atomic<int> completed_queries{0};
    atomic<long long> total_time{0};
    
    auto start_time = chrono::high_resolution_clock::now();
    
    vector<thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&, i]() {
            try {
                MultiFileCatalogV2 catalog(catalog_path);
                
                random_device rd;
                mt19937 gen(rd() + i);  // Different seed per thread
                uniform_real_distribution<> ra_dist(0.0, 5.0);   // Focus on areas with stars
                uniform_real_distribution<> dec_dist(0.0, 5.0);
                uniform_real_distribution<> radius_dist(0.5, 2.0);
                
                for (int q = 0; q < queries_per_thread; q++) {
                    auto query_start = chrono::high_resolution_clock::now();
                    auto stars = catalog.queryCone(ra_dist(gen), dec_dist(gen), radius_dist(gen));
                    auto query_end = chrono::high_resolution_clock::now();
                    
                    auto duration = chrono::duration_cast<chrono::microseconds>(query_end - query_start);
                    total_time += duration.count();
                    completed_queries++;
                }
            } catch (const exception& e) {
                cerr << "Thread " << i << " error: " << e.what() << endl;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    auto wall_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "Results:\n";
    cout << "  Completed queries: " << completed_queries.load() << "\n";
    cout << "  Wall time: " << wall_time.count() << " ms\n";
    cout << "  Average query time: " << (total_time.load() / 1000.0) / completed_queries.load() << " ms\n";
    cout << "  Throughput: " << (completed_queries.load() * 1000.0) / wall_time.count() << " queries/sec\n";
}

void testConcurrentCatalog(const string& catalog_path, int num_threads, int queries_per_thread) {
    cout << "\n⚡ Testing Concurrent MultiFile Catalog\n";
    cout << "═════════════════════════════════════════\n";
    
    atomic<int> completed_queries{0};
    atomic<long long> total_time{0};
    
    // Shared catalog instance for all threads
    ConcurrentMultiFileCatalogV2 catalog(catalog_path, 100);  // Larger cache
    
    auto start_time = chrono::high_resolution_clock::now();
    
    vector<thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&, i]() {
            try {
                random_device rd;
                mt19937 gen(rd() + i);  // Different seed per thread
                uniform_real_distribution<> ra_dist(0.0, 5.0);   // Focus on areas with stars
                uniform_real_distribution<> dec_dist(0.0, 5.0);
                uniform_real_distribution<> radius_dist(0.5, 2.0);
                
                for (int q = 0; q < queries_per_thread; q++) {
                    auto query_start = chrono::high_resolution_clock::now();
                    auto stars = catalog.queryCone(ra_dist(gen), dec_dist(gen), radius_dist(gen));
                    auto query_end = chrono::high_resolution_clock::now();
                    
                    auto duration = chrono::duration_cast<chrono::microseconds>(query_end - query_start);
                    total_time += duration.count();
                    completed_queries++;
                }
            } catch (const exception& e) {
                cerr << "Thread " << i << " error: " << e.what() << endl;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    auto wall_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    auto stats = catalog.getStats();
    
    cout << "Results:\n";
    cout << "  Completed queries: " << completed_queries.load() << "\n";
    cout << "  Wall time: " << wall_time.count() << " ms\n";
    cout << "  Average query time: " << (total_time.load() / 1000.0) / completed_queries.load() << " ms\n";
    cout << "  Throughput: " << (completed_queries.load() * 1000.0) / wall_time.count() << " queries/sec\n";
    cout << "  Cache hit rate: " << stats.hit_rate << "%\n";
    cout << "  Memory used: " << stats.memory_used_mb << " MB\n";
    cout << "  Active readers: " << stats.active_readers << "\n";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <multifile_catalog_directory>" << endl;
        return 1;
    }

    cout << "🔬 Concurrent Catalog Performance Comparison\n";
    cout << "═══════════════════════════════════════════════\n";

    string catalog_path = argv[1];
    
    vector<int> thread_counts = {1, 2, 4, 8};
    int queries_per_thread = 10;
    
    for (int num_threads : thread_counts) {
        cout << "\n📊 Testing with " << num_threads << " threads (" 
             << (num_threads * queries_per_thread) << " total queries)\n";
        cout << "────────────────────────────────────────────────\n";
        
        testOriginalCatalog(catalog_path, num_threads, queries_per_thread);
        testConcurrentCatalog(catalog_path, num_threads, queries_per_thread);
        
        this_thread::sleep_for(chrono::seconds(1));
    }
    
    return 0;
}