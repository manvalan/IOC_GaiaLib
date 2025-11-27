#include "ioc_gaialib/gaia_client.h"
#include <iostream>
#include <sstream>

using namespace ioc::gaia;

int main() {
    std::cout << "Testing Gaia ADQL query for catalog compilation..." << std::endl;
    
    GaiaClient client(GaiaRelease::DR3);
    
    // Test 1: Simple cone query (should work)
    std::cout << "\n[Test 1] Simple cone query around RA=0, Dec=0..." << std::endl;
    try {
        auto stars = client.queryCone(0.0, 0.0, 1.0);
        std::cout << "✅ Found " << stars.size() << " stars" << std::endl;
    } catch (const GaiaException& e) {
        std::cout << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    // Test 2: ADQL query without cross-match (similar to catalog compiler)
    std::cout << "\n[Test 2] ADQL query without cross-match..." << std::endl;
    std::ostringstream adql;
    adql << "SELECT TOP 10 source_id, ra, dec, parallax, parallax_error, "
         << "pmra, pmdec, phot_g_mean_mag, phot_bp_mean_mag, phot_rp_mean_mag "
         << "FROM gaiadr3.gaia_source "
         << "WHERE CONTAINS(POINT('ICRS', ra, dec), "
         << "CIRCLE('ICRS', 0.0, 0.0, 1.0)) = 1 "
         << "AND phot_g_mean_mag < 12.0 "
         << "ORDER BY source_id";
    
    try {
        auto stars = client.queryADQL(adql.str());
        std::cout << "✅ Found " << stars.size() << " stars" << std::endl;
        if (!stars.empty()) {
            std::cout << "   First star: source_id=" << stars[0].source_id 
                     << ", G=" << stars[0].phot_g_mean_mag << std::endl;
        }
    } catch (const GaiaException& e) {
        std::cout << "❌ Error: " << e.what() << std::endl;
        std::cout << "   Query was: " << adql.str() << std::endl;
        return 1;
    }
    
    // Test 3: Try a cross-match query (might fail if tables don't exist as expected)
    std::cout << "\n[Test 3] ADQL query WITH cross-match (SAO only)..." << std::endl;
    std::ostringstream adql_xm;
    adql_xm << "SELECT TOP 10 g.source_id, g.ra, g.dec, g.parallax, g.parallax_error, "
            << "g.pmra, g.pmdec, g.phot_g_mean_mag, g.phot_bp_mean_mag, g.phot_rp_mean_mag, "
            << "sao.sao_id "
            << "FROM gaiadr3.gaia_source AS g "
            << "LEFT OUTER JOIN gaiadr3.sao_neighbourhood AS sao_xm "
            << "ON g.source_id = sao_xm.source_id "
            << "LEFT OUTER JOIN gaiadr3.sao AS sao "
            << "ON sao_xm.sao_id = sao.sao_id "
            << "WHERE CONTAINS(POINT('ICRS', g.ra, g.dec), "
            << "CIRCLE('ICRS', 80.0, 20.0, 1.0)) = 1 "
            << "AND g.phot_g_mean_mag < 10.0";
    
    try {
        auto stars = client.queryADQL(adql_xm.str());
        std::cout << "✅ Found " << stars.size() << " stars with cross-match" << std::endl;
    } catch (const GaiaException& e) {
        std::cout << "⚠️  Cross-match query failed (expected): " << e.what() << std::endl;
        std::cout << "   This is OK - cross-match tables may have different structure" << std::endl;
    }
    
    std::cout << "\n✅ Basic queries work! Catalog compilation should use simpler queries." << std::endl;
    return 0;
}
