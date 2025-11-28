#include "ioc_gaialib/unified_gaia_catalog.h"
#include "ioc_gaialib/common_star_names.h"
#include <iostream>
#include <iomanip>

using namespace ioc::gaia;

void printStar(const GaiaStar& star) {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Gaia DR3 " << star.source_id << std::endl;
    std::cout << "  Position: RA=" << star.ra << "°, Dec=" << star.dec << "°" << std::endl;
    std::cout << "  Magnitude: G=" << star.phot_g_mean_mag << std::endl;
    std::cout << "  Parallax: " << star.parallax << " mas" << std::endl;
    
    // Show all designations using the new method
    std::string all_designations = star.getAllDesignations();
    std::cout << "  All designations: " << all_designations << std::endl;
    std::cout << std::endl;
}

void testCrossmatchQueries() {
    std::cout << "=== Testing Cross-Match Queries ===" << std::endl;
    
    try {
        // Initialize catalog (in real usage, you would configure properly)
        auto& catalog = UnifiedGaiaCatalog::getInstance();
        
        // Test queries by different catalog designations
        struct TestQuery {
            std::string type;
            std::string designation;
            std::string description;
        };
        
        std::vector<TestQuery> queries = {
            {"NAME", "Sirius", "Brightest star in the sky"},
            {"NAME", "Vega", "Standard star for magnitude scale"},
            {"NAME", "Polaris", "North Star"},
            {"NAME", "Arcturus", "Brightest star in northern hemisphere"},
            {"SAO", "151881", "Sirius by SAO designation"},
            {"HD", "48915", "Sirius by HD designation"},
            {"HIP", "32349", "Sirius by Hipparcos designation"},
            {"HD", "172167", "Vega by HD designation"},
            {"NAME", "Betelgeuse", "Red supergiant in Orion"},
            {"NAME", "Rigel", "Blue supergiant in Orion"}
        };
        
        for (const auto& query : queries) {
            std::cout << "--- Searching for " << query.description << " ---" << std::endl;
            std::cout << "Query: " << query.type << " " << query.designation << std::endl;
            
            std::optional<GaiaStar> result;
            
            if (query.type == "NAME") {
                result = catalog.queryByName(query.designation);
            } else if (query.type == "SAO") {
                result = catalog.queryBySAO(query.designation);
            } else if (query.type == "HD") {
                result = catalog.queryByHD(query.designation);
            } else if (query.type == "HIP") {
                result = catalog.queryByHipparcos(query.designation);
            }
            
            if (result.has_value()) {
                std::cout << "✓ Found!" << std::endl;
                printStar(result.value());
            } else {
                std::cout << "✗ Not found in catalog" << std::endl << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void testEmbeddedDatabase() {
    std::cout << "=== Testing Embedded Star Database ===" << std::endl;
    
    // Test the CommonStarNames class directly
    CommonStarNames star_names;
    if (star_names.loadDefaultDatabase()) {
        std::cout << "✓ Embedded database loaded successfully" << std::endl;
        
        // Get all star names
        auto all_names = star_names.getAllNames();
        std::cout << "Database contains " << all_names.size() << " stars:" << std::endl;
        
        // Show first 10 names
        std::cout << "First 10 stars:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(10), all_names.size()); ++i) {
            std::cout << "  " << (i+1) << ". " << all_names[i] << std::endl;
        }
        
        std::cout << std::endl;
        
        // Test specific lookups
        std::vector<std::string> test_names = {"Sirius", "Vega", "Polaris", "Betelgeuse"};
        
        for (const auto& name : test_names) {
            auto source_id = star_names.findByName(name);
            if (source_id.has_value()) {
                auto cross_match = star_names.getCrossMatch(source_id.value());
                if (cross_match.has_value()) {
                    std::cout << name << ":" << std::endl;
                    std::cout << "  Gaia source_id: " << source_id.value() << std::endl;
                    std::cout << "  SAO: " << cross_match->sao_designation << std::endl;
                    std::cout << "  HD: " << cross_match->hd_designation << std::endl;
                    std::cout << "  HIP: " << cross_match->hip_designation << std::endl;
                    std::cout << "  TYC: " << cross_match->tycho2_designation << std::endl;
                    std::cout << "  Constellation: " << cross_match->constellation << std::endl;
                    if (!cross_match->notes.empty()) {
                        std::cout << "  Notes: " << cross_match->notes << std::endl;
                    }
                    std::cout << std::endl;
                }
            }
        }
        
    } else {
        std::cout << "✗ Failed to load embedded database" << std::endl;
    }
}

int main() {
    std::cout << "IOC GaiaLib - Cross-Match System Test" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;
    
    // Test the embedded database first
    testEmbeddedDatabase();
    
    // Then test the full catalog queries
    // Note: This will only work if you have a catalog file configured
    testCrossmatchQueries();
    
    return 0;
}