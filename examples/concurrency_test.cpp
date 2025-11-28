#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include "../include/ioc_gaialib/multifile_catalog_v2.h"

using namespace ioc::gaia;
using namespace std;

class ConcurrencyTester {
private:
    string catalog_path_;
    atomic<int> successful_queries_{0};
    atomic<int> failed_queries_{0};
    atomic<long long> total_time_us_{0};
    atomic<int> total_stars_found_{0};

public:
    ConcurrencyTester(const string& catalog_path) : catalog_path_(catalog_path) {}

    void workerThread(int thread_id, int num_queries) {
        try {
            // Each thread creates its own catalog instance
            cout << "Thread " << thread_id << ": Creating catalog instance..." << endl;
            MultiFileCatalogV2 catalog(catalog_path_);
            
            // Random number generator for coordinates
            random_device rd;
            mt19937 gen(rd());
            uniform_real_distribution<> ra_dist(0.0, 360.0);
            uniform_real_distribution<> dec_dist(-90.0, 90.0);
            uniform_real_distribution<> radius_dist(0.1, 2.0);
            
            for (int i = 0; i < num_queries; i++) {
                try {
                    double ra = ra_dist(gen);
                    double dec = dec_dist(gen);
                    double radius = radius_dist(gen);
                    
                    auto start = chrono::high_resolution_clock::now();
                    auto stars = catalog.queryCone(ra, dec, radius);
                    auto end = chrono::high_resolution_clock::now();
                    
                    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                    
                    successful_queries_++;
                    total_time_us_ += duration.count();
                    total_stars_found_ += stars.size();
                    
                    if (i % 10 == 0) {
                        cout << "Thread " << thread_id << ": Query " << i 
                             << " -> " << stars.size() << " stars in " 
                             << duration.count() << " μs" << endl;
                    }
                    
                } catch (const exception& e) {
                    failed_queries_++;
                    cerr << "Thread " << thread_id << " query " << i 
                         << " failed: " << e.what() << endl;
                }
            }
            
            cout << "Thread " << thread_id << ": Completed " << num_queries << " queries" << endl;
            
        } catch (const exception& e) {
            cerr << "Thread " << thread_id << " failed to create catalog: " << e.what() << endl;
            failed_queries_ += num_queries;
        }
    }

    void runConcurrencyTest(int num_threads, int queries_per_thread) {
        cout << "\n🧪 Starting Concurrency Test\n";
        cout << "═══════════════════════════════\n";
        cout << "Threads: " << num_threads << "\n";
        cout << "Queries per thread: " << queries_per_thread << "\n";
        cout << "Total queries: " << (num_threads * queries_per_thread) << "\n\n";

        vector<thread> threads;
        auto start_time = chrono::high_resolution_clock::now();

        // Launch all threads
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(&ConcurrencyTester::workerThread, this, i, queries_per_thread);
        }

        // Wait for all threads to complete
        for (auto& t : threads) {
            t.join();
        }

        auto end_time = chrono::high_resolution_clock::now();
        auto total_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

        // Print results
        cout << "\n📊 Concurrency Test Results\n";
        cout << "══════════════════════════════\n";
        cout << "✅ Successful queries: " << successful_queries_.load() << "\n";
        cout << "❌ Failed queries: " << failed_queries_.load() << "\n";
        cout << "⏱️  Total time: " << total_duration.count() << " ms\n";
        cout << "🌟 Total stars found: " << total_stars_found_.load() << "\n";
        
        if (successful_queries_.load() > 0) {
            double avg_time = total_time_us_.load() / double(successful_queries_.load());
            cout << "⚡ Average query time: " << avg_time << " μs (" << avg_time/1000.0 << " ms)\n";
            cout << "🚀 Queries per second: " << (successful_queries_.load() * 1000.0) / total_duration.count() << "\n";
        }
        
        double success_rate = (successful_queries_.load() * 100.0) / (successful_queries_.load() + failed_queries_.load());
        cout << "📈 Success rate: " << success_rate << "%\n";
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <multifile_catalog_directory>" << endl;
        return 1;
    }

    ConcurrencyTester tester(argv[1]);
    
    // Test with increasing concurrency
    vector<int> thread_counts = {1, 2, 4, 8};
    int queries_per_thread = 20;
    
    for (int num_threads : thread_counts) {
        tester.runConcurrencyTest(num_threads, queries_per_thread);
        this_thread::sleep_for(chrono::seconds(2)); // Cool down between tests
    }
    
    cout << "\n🎯 Concurrency testing completed!\n";
    return 0;
}