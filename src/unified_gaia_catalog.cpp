#include "ioc_gaialib/unified_gaia_catalog.h"
#include "ioc_gaialib/concurrent_multifile_catalog_v2.h"
#include "ioc_gaialib/gaia_mag18_catalog.h"
#include "ioc_gaialib/gaia_client.h"
#include "ioc_gaialib/common_star_names.h"
#include "ioc_gaialib/iau_star_catalog_parser.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <atomic>

namespace ioc::gaia {

// Simple JSON parser to avoid external dependencies
class SimpleJSON {
public:
    static std::map<std::string, std::string> parse(const std::string& json_str) {
        std::map<std::string, std::string> result;
        
        std::string clean_str = json_str;
        
        // Remove all whitespace except within quoted strings
        std::string cleaned;
        bool in_quotes = false;
        for (char c : clean_str) {
            if (c == '"') {
                in_quotes = !in_quotes;
                cleaned += c;
            } else if (in_quotes || (c != ' ' && c != '\t' && c != '\n' && c != '\r')) {
                cleaned += c;
            }
        }
        
        // Remove { } 
        size_t start = cleaned.find('{');
        size_t end = cleaned.find_last_of('}');
        if (start != std::string::npos && end != std::string::npos) {
            cleaned = cleaned.substr(start + 1, end - start - 1);
        }
        
        // Split by comma
        std::stringstream ss(cleaned);
        std::string item;
        
        while (std::getline(ss, item, ',')) {
            size_t colon = item.find(':');
            if (colon != std::string::npos) {
                std::string key = item.substr(0, colon);
                std::string value = item.substr(colon + 1);
                
                // Remove quotes
                key = removeQuotes(key);
                value = removeQuotes(value);
                
                result[key] = value;
            }
        }
        
        return result;
    }
    
private:
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
    
    static std::string removeQuotes(const std::string& str) {
        std::string result = str;
        if (result.front() == '"' && result.back() == '"') {
            result = result.substr(1, result.length() - 2);
        }
        return result;
    }
};

// Private implementation class
class UnifiedGaiaCatalog::Impl {
public:
    GaiaCatalogConfig config_;
    
    // Catalog implementations
    std::unique_ptr<ConcurrentMultiFileCatalogV2> multifile_catalog_;
    std::unique_ptr<ioc_gaialib::GaiaMag18Catalog> compressed_catalog_;
    std::unique_ptr<GaiaClient> online_client_;
    
    // Star names cross-match system
    CommonStarNames star_names_;
    bool star_names_loaded_{false};
    
    // Statistics
    std::atomic<size_t> total_queries_{0};
    std::atomic<size_t> total_stars_returned_{0};
    std::atomic<double> total_query_time_{0.0};
    
    Impl() {
        // Try to load star names database on construction
        try {
            initializeStarNames();
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to initialize star names database: " << e.what() << std::endl;
        }
    }
    
    bool initializeMultiFile(const std::string& directory) {
        try {
            multifile_catalog_ = std::make_unique<ConcurrentMultiFileCatalogV2>(
                directory, config_.max_cached_chunks
            );
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize multi-file catalog: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool initializeCompressed(const std::string& file_path) {
        try {
            compressed_catalog_ = std::make_unique<ioc_gaialib::GaiaMag18Catalog>(file_path);
            return compressed_catalog_->isLoaded();
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize compressed catalog: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool initializeOnline() {
        try {
            online_client_ = std::make_unique<GaiaClient>();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize online client: " << e.what() << std::endl;
            return false;
        }
    }
    
    std::vector<GaiaStar> performQuery(const QueryParams& params) {
        auto start_time = std::chrono::high_resolution_clock::now();
        total_queries_++;
        
        std::vector<GaiaStar> results;
        
        try {
            switch (config_.catalog_type) {
                case GaiaCatalogConfig::CatalogType::MULTIFILE_V2:
                    if (multifile_catalog_) {
                        results = multifile_catalog_->queryCone(
                            params.ra_center, params.dec_center, params.radius
                        );
                    }
                    break;
                    
                case GaiaCatalogConfig::CatalogType::COMPRESSED_V2:
                    if (compressed_catalog_) {
                        results = compressed_catalog_->queryCone(
                            params.ra_center, params.dec_center, params.radius
                        );
                    }
                    break;
                    
                case GaiaCatalogConfig::CatalogType::ONLINE_ESA:
                    if (online_client_) {
                        results = online_client_->queryCone(
                            params.ra_center, params.dec_center, params.radius,
                            params.max_magnitude
                        );
                    }
                    break;
                    
                case GaiaCatalogConfig::CatalogType::ONLINE_VIZIER:
                    // TODO: Implement VizieR support
                    throw std::runtime_error("VizieR support not yet implemented");
                    break;
            }
            
            // Apply additional filters
            if (params.max_magnitude > 0) {
                results.erase(
                    std::remove_if(results.begin(), results.end(),
                        [&](const GaiaStar& star) {
                            return star.phot_g_mean_mag > params.max_magnitude;
                        }),
                    results.end()
                );
            }
            
            if (params.min_parallax >= 0) {
                results.erase(
                    std::remove_if(results.begin(), results.end(),
                        [&](const GaiaStar& star) {
                            return star.parallax < params.min_parallax;
                        }),
                    results.end()
                );
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Query failed: " << e.what() << std::endl;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        double duration_ms = duration.count();
        
        // Atomic operations for statistics
        double old_time = total_query_time_.load();
        while (!total_query_time_.compare_exchange_weak(old_time, old_time + duration_ms)) {
            // Retry if another thread modified the value
        }
        total_stars_returned_ += results.size();
        
        return results;
    }
    
    std::optional<GaiaStar> queryBySourceId(uint64_t source_id) {
        auto result = performSourceIdQuery(source_id);
        if (result.has_value()) {
            // Add cross-match information if available
            if (star_names_loaded_) {
                auto cross_match = star_names_.getCrossMatch(source_id);
                if (cross_match.has_value()) {
                    result->sao_designation = cross_match->sao_designation;
                    result->hd_designation = cross_match->hd_designation;
                    result->hip_designation = cross_match->hip_designation;
                    result->tycho2_designation = cross_match->tycho2_designation;
                    result->common_name = cross_match->common_name;
                }
            }
        }
        return result;
    }
    
    std::optional<GaiaStar> performSourceIdQuery(uint64_t source_id) {
        switch (config_.catalog_type) {
            case GaiaCatalogConfig::CatalogType::MULTIFILE_V2:
                if (multifile_catalog_) {
                    return multifile_catalog_->queryBySourceId(source_id);
                }
                break;
                
            case GaiaCatalogConfig::CatalogType::COMPRESSED_V2:
                if (compressed_catalog_) {
                    return compressed_catalog_->queryBySourceId(source_id);
                }
                break;
                
            case GaiaCatalogConfig::CatalogType::ONLINE_ESA:
                // Online queries by source ID would need special implementation
                break;
                
            case GaiaCatalogConfig::CatalogType::ONLINE_VIZIER:
                break;
        }
        return std::nullopt;
    }
    
    std::optional<GaiaStar> queryByCatalogDesignation(const std::string& catalog_type, const std::string& designation) {
        if (!star_names_loaded_) {
            return std::nullopt;
        }
        
        std::optional<uint64_t> source_id;
        
        // Find source_id based on catalog type
        if (catalog_type == "SAO") {
            source_id = star_names_.findBySAO(designation);
        } else if (catalog_type == "HD") {
            source_id = star_names_.findByHD(designation);
        } else if (catalog_type == "HIP") {
            source_id = star_names_.findByHipparcos(designation);
        } else if (catalog_type == "TYC") {
            source_id = star_names_.findByTycho2(designation);
        } else if (catalog_type == "NAME") {
            source_id = star_names_.findByName(designation);
        }
        
        if (!source_id.has_value()) {
            return std::nullopt;
        }
        
        // Query by source_id and populate cross-match information
        return queryBySourceId(source_id.value());
    }

private:
    void initializeStarNames() {
        try {
            // Try to load from IAU catalog first (if exists)
            if (IAUStarCatalogParser::loadFromJSON("data/IAU-CSN.json", star_names_)) {
                star_names_loaded_ = true;
                std::cout << "Loaded star names from official IAU catalog" << std::endl;
                return;
            }
            
            // Try to load from custom CSV file
            if (star_names_.loadDatabase("data/common_star_names.csv")) {
                star_names_loaded_ = true;
                std::cout << "Loaded star names from custom CSV file" << std::endl;
                return;
            } 
            
            // Fall back to embedded database
            if (star_names_.loadDefaultDatabase()) {
                star_names_loaded_ = true;
                std::cout << "Loaded star names from embedded database" << std::endl;
            } else {
                std::cerr << "Warning: Could not load star names database" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error initializing star names: " << e.what() << std::endl;
        }
    }
};

// Static members
std::unique_ptr<UnifiedGaiaCatalog> UnifiedGaiaCatalog::instance_{nullptr};
bool UnifiedGaiaCatalog::initialized_{false};

bool UnifiedGaiaCatalog::initialize(const std::string& json_config) {
    try {
        auto config_map = SimpleJSON::parse(json_config);
        
        // Create instance if needed
        if (!instance_) {
            instance_ = std::unique_ptr<UnifiedGaiaCatalog>(new UnifiedGaiaCatalog());
            instance_->pimpl_ = std::make_unique<Impl>();
        }
        
        auto& impl = *instance_->pimpl_;
        
        // Parse configuration
        std::string catalog_type_str = config_map["catalog_type"];
        if (catalog_type_str == "multifile_v2") {
            impl.config_.catalog_type = GaiaCatalogConfig::CatalogType::MULTIFILE_V2;
            impl.config_.multifile_directory = config_map["multifile_directory"];
            
            if (config_map.find("max_cached_chunks") != config_map.end()) {
                impl.config_.max_cached_chunks = std::stoull(config_map["max_cached_chunks"]);
            }
            
            if (!impl.initializeMultiFile(impl.config_.multifile_directory)) {
                return false;
            }
            
        } else if (catalog_type_str == "compressed_v2") {
            impl.config_.catalog_type = GaiaCatalogConfig::CatalogType::COMPRESSED_V2;
            impl.config_.compressed_file_path = config_map["compressed_file_path"];
            
            if (!impl.initializeCompressed(impl.config_.compressed_file_path)) {
                return false;
            }
            
        } else if (catalog_type_str == "online_esa") {
            impl.config_.catalog_type = GaiaCatalogConfig::CatalogType::ONLINE_ESA;
            
            if (config_map.find("timeout_seconds") != config_map.end()) {
                impl.config_.timeout_seconds = std::stoi(config_map["timeout_seconds"]);
            }
            
            if (!impl.initializeOnline()) {
                return false;
            }
            
        } else {
            std::cerr << "Unknown catalog type: " << catalog_type_str << std::endl;
            return false;
        }
        
        initialized_ = true;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        return false;
    }
}

UnifiedGaiaCatalog& UnifiedGaiaCatalog::getInstance() {
    if (!initialized_ || !instance_) {
        throw std::runtime_error("UnifiedGaiaCatalog not initialized. Call initialize() first.");
    }
    return *instance_;
}

void UnifiedGaiaCatalog::shutdown() {
    if (instance_) {
        instance_.reset();
        initialized_ = false;
    }
}

bool UnifiedGaiaCatalog::isInitialized() const {
    return pimpl_ != nullptr;
}

CatalogInfo UnifiedGaiaCatalog::getCatalogInfo() const {
    if (!pimpl_) {
        throw std::runtime_error("Catalog not initialized");
    }
    
    CatalogInfo info;
    
    switch (pimpl_->config_.catalog_type) {
        case GaiaCatalogConfig::CatalogType::MULTIFILE_V2:
            info.catalog_name = "Multi-File Gaia DR3 V2";
            info.version = "2.0";
            info.magnitude_limit = 21.0;
            info.is_online = false;
            break;
            
        case GaiaCatalogConfig::CatalogType::COMPRESSED_V2:
            info.catalog_name = "Compressed Gaia DR3 V2";
            info.version = "2.0";
            info.magnitude_limit = 18.0;
            info.is_online = false;
            break;
            
        case GaiaCatalogConfig::CatalogType::ONLINE_ESA:
            info.catalog_name = "ESA Gaia Archive";
            info.version = "DR3";
            info.magnitude_limit = 21.0;
            info.is_online = true;
            break;
            
        case GaiaCatalogConfig::CatalogType::ONLINE_VIZIER:
            info.catalog_name = "VizieR Gaia DR3";
            info.version = "DR3";
            info.magnitude_limit = 21.0;
            info.is_online = true;
            break;
    }
    
    info.available_fields = {
        "source_id", "ra", "dec", "phot_g_mean_mag", 
        "phot_bp_mean_mag", "phot_rp_mean_mag", "parallax"
    };
    
    return info;
}

std::vector<GaiaStar> UnifiedGaiaCatalog::queryCone(const QueryParams& params) const {
    if (!pimpl_) {
        throw std::runtime_error("Catalog not initialized");
    }
    return pimpl_->performQuery(params);
}

std::future<std::vector<GaiaStar>> UnifiedGaiaCatalog::queryAsync(
    const QueryParams& params,
    ProgressCallback progress_callback
) const {
    return std::async(std::launch::async, [this, params, progress_callback]() {
        if (progress_callback) {
            progress_callback(0, "Starting query...");
        }
        
        auto result = queryCone(params);
        
        if (progress_callback) {
            progress_callback(100, "Query completed");
        }
        
        return result;
    });
}

std::vector<std::vector<GaiaStar>> UnifiedGaiaCatalog::batchQuery(
    const std::vector<QueryParams>& param_list
) const {
    std::vector<std::vector<GaiaStar>> results;
    results.reserve(param_list.size());
    
    for (const auto& params : param_list) {
        results.push_back(queryCone(params));
    }
    
    return results;
}

std::optional<GaiaStar> UnifiedGaiaCatalog::queryBySourceId(uint64_t source_id) const {
    if (!pimpl_) {
        throw std::runtime_error("Catalog not initialized");
    }
    return pimpl_->queryBySourceId(source_id);
}

std::optional<GaiaStar> UnifiedGaiaCatalog::queryBySAO(const std::string& sao_number) const {
    if (!pimpl_) {
        throw std::runtime_error("Catalog not initialized");
    }
    return pimpl_->queryByCatalogDesignation("SAO", sao_number);
}

std::optional<GaiaStar> UnifiedGaiaCatalog::queryByTycho2(const std::string& tycho2_designation) const {
    if (!pimpl_) {
        throw std::runtime_error("Catalog not initialized");
    }
    return pimpl_->queryByCatalogDesignation("TYC", tycho2_designation);
}

std::optional<GaiaStar> UnifiedGaiaCatalog::queryByHD(const std::string& hd_number) const {
    if (!pimpl_) {
        throw std::runtime_error("Catalog not initialized");
    }
    return pimpl_->queryByCatalogDesignation("HD", hd_number);
}

std::optional<GaiaStar> UnifiedGaiaCatalog::queryByHipparcos(const std::string& hip_number) const {
    if (!pimpl_) {
        throw std::runtime_error("Catalog not initialized");
    }
    return pimpl_->queryByCatalogDesignation("HIP", hip_number);
}

std::optional<GaiaStar> UnifiedGaiaCatalog::queryByName(const std::string& common_name) const {
    if (!pimpl_) {
        throw std::runtime_error("Catalog not initialized");
    }
    return pimpl_->queryByCatalogDesignation("NAME", common_name);
}

CatalogStats UnifiedGaiaCatalog::getStatistics() const {
    if (!pimpl_) {
        throw std::runtime_error("Catalog not initialized");
    }
    
    CatalogStats stats;
    stats.total_queries = pimpl_->total_queries_.load();
    stats.total_stars_returned = pimpl_->total_stars_returned_.load();
    
    if (stats.total_queries > 0) {
        stats.average_query_time_ms = pimpl_->total_query_time_.load() / stats.total_queries;
    }
    
    // Cache hit rate calculation would depend on specific catalog implementation
    stats.cache_hit_rate = 85.0; // Placeholder
    stats.memory_used_mb = 512;   // Placeholder
    stats.disk_cache_used_mb = 0; // Placeholder
    
    return stats;
}

void UnifiedGaiaCatalog::clearCache() {
    if (!pimpl_) {
        return;
    }
    
    // Clear caches based on catalog type
    switch (pimpl_->config_.catalog_type) {
        case GaiaCatalogConfig::CatalogType::MULTIFILE_V2:
            if (pimpl_->multifile_catalog_) {
                pimpl_->multifile_catalog_->clearCache();
            }
            break;
        // Other catalog types would implement clearCache differently
        case GaiaCatalogConfig::CatalogType::COMPRESSED_V2:
        case GaiaCatalogConfig::CatalogType::ONLINE_ESA:
        case GaiaCatalogConfig::CatalogType::ONLINE_VIZIER:
            break;
    }
}

bool UnifiedGaiaCatalog::reconfigure(const std::string& json_config) {
    shutdown();
    return initialize(json_config);
}

} // namespace ioc::gaia