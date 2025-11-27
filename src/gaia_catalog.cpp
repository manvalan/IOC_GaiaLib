#include "ioc_gaialib/gaia_catalog.h"
#include <chrono>
#include <iostream>
#include <algorithm>

namespace ioc_gaialib {

GaiaCatalog::GaiaCatalog(const GaiaCatalogConfig& config)
    : config_(config), verbose_(false) {
    
    // Initialize statistics
    stats_ = Statistics{};
    stats_.total_queries = 0;
    stats_.mag18_queries = 0;
    stats_.grappa_queries = 0;
    stats_.online_queries = 0;
    stats_.cache_hits = 0;
    stats_.cache_misses = 0;
    
    // Load Mag18 catalog if path provided
    if (!config_.mag18_catalog_path.empty()) {
        try {
            mag18_catalog_ = std::make_unique<GaiaMag18Catalog>(config_.mag18_catalog_path);
            if (mag18_catalog_->isLoaded()) {
                auto mag18_stats = mag18_catalog_->getStatistics();
                stats_.mag18_stars_available = mag18_stats.total_stars;
                stats_.mag18_loaded = true;
                
                if (verbose_) {
                    std::cout << "✅ Mag18 catalog loaded: " 
                              << stats_.mag18_stars_available << " stars\n";
                }
            } else {
                mag18_catalog_.reset();
                stats_.mag18_loaded = false;
            }
        } catch (const std::exception& e) {
            if (verbose_) {
                std::cerr << "⚠️  Failed to load Mag18 catalog: " << e.what() << "\n";
            }
            stats_.mag18_loaded = false;
        }
    } else {
        stats_.mag18_loaded = false;
    }
    
    // Load GRAPPA3E catalog if path provided
    if (!config_.grappa_catalog_path.empty()) {
        try {
            grappa_catalog_ = std::make_unique<GaiaLocalCatalog>(config_.grappa_catalog_path);
            if (grappa_catalog_->isLoaded()) {
                stats_.grappa_loaded = true;
                // GRAPPA3E has all 1.8B stars
                stats_.grappa_stars_available = 1800000000;
                
                if (verbose_) {
                    std::cout << "✅ GRAPPA3E catalog loaded: " 
                              << stats_.grappa_stars_available << " stars\n";
                }
            } else {
                grappa_catalog_.reset();
                stats_.grappa_loaded = false;
            }
        } catch (const std::exception& e) {
            if (verbose_) {
                std::cerr << "⚠️  Failed to load GRAPPA3E catalog: " << e.what() << "\n";
            }
            stats_.grappa_loaded = false;
        }
    } else {
        stats_.grappa_loaded = false;
    }
    
    // Initialize online client if enabled
    if (config_.enable_online_fallback) {
        try {
            online_client_ = std::make_unique<ioc::gaia::GaiaClient>(config_.online_release);
            stats_.online_enabled = true;
            
            if (verbose_) {
                std::cout << "✅ Online fallback enabled\n";
            }
        } catch (const std::exception& e) {
            if (verbose_) {
                std::cerr << "⚠️  Failed to enable online client: " << e.what() << "\n";
            }
            stats_.online_enabled = false;
        }
    } else {
        stats_.online_enabled = false;
    }
}

GaiaCatalog::~GaiaCatalog() {
    // Cleanup handled by unique_ptr
}

bool GaiaCatalog::isReady() const {
    return stats_.mag18_loaded || stats_.grappa_loaded || stats_.online_enabled;
}

GaiaCatalog::Statistics GaiaCatalog::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void GaiaCatalog::updateCache(uint64_t source_id, const ioc::gaia::GaiaStar& star) {
    if (!config_.enable_caching) return;
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Simple LRU: if cache full, remove oldest (first element)
    if (star_cache_.size() >= config_.cache_size) {
        star_cache_.erase(star_cache_.begin());
    }
    
    star_cache_[source_id] = star;
}

std::optional<ioc::gaia::GaiaStar> GaiaCatalog::checkCache(uint64_t source_id) const {
    if (!config_.enable_caching) return std::nullopt;
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = star_cache_.find(source_id);
    if (it != star_cache_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

void GaiaCatalog::logQuery(const std::string& method, const std::string& source,
                            double duration_ms, size_t results) const {
    if (verbose_) {
        std::cout << "Query: " << method << " via " << source 
                  << " - " << duration_ms << " ms - " << results << " results\n";
    }
}

std::optional<ioc::gaia::GaiaStar> GaiaCatalog::queryStar(uint64_t source_id) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_queries++;
    
    // 1. Check cache first
    auto cached = checkCache(source_id);
    if (cached) {
        stats_.cache_hits++;
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"cache", duration, 1};
        logQuery("queryStar", "cache", duration, 1);
        return cached;
    }
    
    stats_.cache_misses++;
    
    // 2. Try Mag18 (fastest for source_id lookup)
    if (config_.prefer_mag18_for_source_id && mag18_catalog_) {
        auto star = mag18_catalog_->queryBySourceId(source_id);
        if (star) {
            stats_.mag18_queries++;
            updateCache(source_id, *star);
            
            auto end = std::chrono::high_resolution_clock::now();
            double duration = std::chrono::duration<double, std::milli>(end - start).count();
            
            last_metrics_ = {"mag18", duration, 1};
            logQuery("queryStar", "mag18", duration, 1);
            return star;
        }
    }
    
    // 3. Try GRAPPA3E (has all stars, but slower lookup)
    if (grappa_catalog_) {
        auto star = grappa_catalog_->queryBySourceId(source_id);
        if (star) {
            stats_.grappa_queries++;
            updateCache(source_id, *star);
            
            auto end = std::chrono::high_resolution_clock::now();
            double duration = std::chrono::duration<double, std::milli>(end - start).count();
            
            last_metrics_ = {"grappa", duration, 1};
            logQuery("queryStar", "grappa", duration, 1);
            return star;
        }
    }
    
    // 4. Try online fallback
    if (online_client_) {
        // TODO: Implement online query by source_id
        stats_.online_queries++;
        // For now, return nullopt
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();
    last_metrics_ = {"none", duration, 0};
    
    return std::nullopt;
}

std::vector<ioc::gaia::GaiaStar> GaiaCatalog::queryCone(
    double ra, double dec, double radius, size_t max_results) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_queries++;
    
    std::vector<ioc::gaia::GaiaStar> results;
    
    // ALWAYS prefer GRAPPA3E for cone search (1000x faster)
    if (grappa_catalog_) {
        results = grappa_catalog_->queryCone(ra, dec, radius, max_results);
        stats_.grappa_queries++;
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"grappa", duration, results.size()};
        logQuery("queryCone", "grappa", duration, results.size());
        return results;
    }
    
    // Fallback to Mag18 if GRAPPA3E not available
    if (mag18_catalog_) {
        results = mag18_catalog_->queryCone(ra, dec, radius, max_results);
        stats_.mag18_queries++;
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"mag18", duration, results.size()};
        logQuery("queryCone", "mag18", duration, results.size());
        return results;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();
    last_metrics_ = {"none", duration, 0};
    
    return results;
}

std::vector<ioc::gaia::GaiaStar> GaiaCatalog::queryConeWithMagnitude(
    double ra, double dec, double radius,
    double mag_min, double mag_max, size_t max_results) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_queries++;
    
    std::vector<ioc::gaia::GaiaStar> results;
    
    // ALWAYS prefer GRAPPA3E for spatial queries (much faster)
    if (grappa_catalog_) {
        results = grappa_catalog_->queryConeWithMagnitude(ra, dec, radius, mag_min, mag_max, max_results);
        stats_.grappa_queries++;
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"grappa", duration, results.size()};
        logQuery("queryConeWithMagnitude", "grappa", duration, results.size());
        return results;
    }
    
    // Fallback to Mag18 (will only return stars ≤ 18)
    if (mag18_catalog_) {
        results = mag18_catalog_->queryConeWithMagnitude(ra, dec, radius, mag_min, mag_max, max_results);
        stats_.mag18_queries++;
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"mag18", duration, results.size()};
        logQuery("queryConeWithMagnitude", "mag18", duration, results.size());
        return results;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();
    last_metrics_ = {"none", duration, 0};
    
    return results;
}

std::vector<ioc::gaia::GaiaStar> GaiaCatalog::queryBrightest(
    double ra, double dec, double radius, size_t n_brightest) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_queries++;
    
    std::vector<ioc::gaia::GaiaStar> results;
    
    // Prefer GRAPPA3E for brightest queries (faster for large regions)
    if (grappa_catalog_) {
        results = grappa_catalog_->queryBrightest(ra, dec, radius, n_brightest);
        stats_.grappa_queries++;
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"grappa", duration, results.size()};
        logQuery("queryBrightest", "grappa", duration, results.size());
        return results;
    }
    
    // Fallback to Mag18
    if (mag18_catalog_) {
        results = mag18_catalog_->queryBrightest(ra, dec, radius, n_brightest);
        stats_.mag18_queries++;
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"mag18", duration, results.size()};
        logQuery("queryBrightest", "mag18", duration, results.size());
        return results;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();
    last_metrics_ = {"none", duration, 0};
    
    return results;
}

std::vector<ioc::gaia::GaiaStar> GaiaCatalog::queryBox(
    double ra_min, double ra_max, double dec_min, double dec_max, size_t max_results) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_queries++;
    
    std::vector<ioc::gaia::GaiaStar> results;
    
    // Only GRAPPA3E supports box queries
    if (grappa_catalog_) {
        results = grappa_catalog_->queryBox(ra_min, ra_max, dec_min, dec_max, max_results);
        stats_.grappa_queries++;
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"grappa", duration, results.size()};
        logQuery("queryBox", "grappa", duration, results.size());
        return results;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();
    last_metrics_ = {"none", duration, 0};
    
    return results;
}

size_t GaiaCatalog::countInCone(double ra, double dec, double radius) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_queries++;
    
    size_t count = 0;
    
    // Prefer GRAPPA3E for counting (faster)
    if (grappa_catalog_) {
        count = grappa_catalog_->countInCone(ra, dec, radius);
        stats_.grappa_queries++;
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"grappa", duration, count};
        logQuery("countInCone", "grappa", duration, count);
        return count;
    }
    
    // Fallback to Mag18
    if (mag18_catalog_) {
        count = mag18_catalog_->countInCone(ra, dec, radius);
        stats_.mag18_queries++;
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        last_metrics_ = {"mag18", duration, count};
        logQuery("countInCone", "mag18", duration, count);
        return count;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();
    last_metrics_ = {"none", duration, 0};
    
    return count;
}

std::map<uint64_t, ioc::gaia::GaiaStar> GaiaCatalog::queryStars(
    const std::vector<uint64_t>& source_ids) {
    
    std::map<uint64_t, ioc::gaia::GaiaStar> results;
    
    for (uint64_t source_id : source_ids) {
        auto star = queryStar(source_id);
        if (star) {
            results[source_id] = *star;
        }
    }
    
    return results;
}

void GaiaCatalog::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    star_cache_.clear();
}

std::pair<size_t, size_t> GaiaCatalog::getCacheStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return {stats_.cache_hits, stats_.cache_misses};
}

} // namespace ioc_gaialib
