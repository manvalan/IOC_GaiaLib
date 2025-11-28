#include "ioc_gaialib/gaia_mag18_catalog.h"
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <chrono>

using namespace ioc_gaialib;
using namespace ioc::gaia;

uint32_t computeHEALPixPixel(double ra, double dec, uint32_t nside) {
    const double DEG2RAD = 3.14159265358979323846 / 180.0;
    const double PI = 3.14159265358979323846;
    const double TWOPI = 2.0 * PI;
    
    double theta = (90.0 - dec) * DEG2RAD;
    double phi = ra * DEG2RAD;
    
    const double z = cos(theta);
    const double za = fabs(z);
    
    // Simplified HEALPix ang2pix_nest for NSIDE=64
    if (za <= 2.0/3.0) {
        const double temp1 = nside * (0.5 + phi / TWOPI);
        const double temp2 = nside * z * 0.75;
        const uint32_t jp = static_cast<uint32_t>(temp1 - temp2);
        const uint32_t jm = static_cast<uint32_t>(temp1 + temp2);
        const uint32_t ir = nside + 1 + jp - jm;
        const uint32_t kshift = 1 - (ir & 1);
        const uint32_t ip = (jp + jm - nside + kshift + 1) / 2;
        
        uint32_t face_num = (ir - nside) / nside;
        uint32_t ix = ip % nside;
        uint32_t iy = (ir - 1) % nside;
        
        return (face_num * nside * nside) + (ix + iy * nside);
    }
    
    // Polar caps
    return static_cast<uint32_t>(phi / TWOPI * 4 * nside);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <v1_catalog.cat.gz> <output_v2.mag18v2>\n";
        std::cout << "\nConverts existing Mag18 V1 catalog to V2 format with HEALPix index\n";
        std::cout << "This is MUCH faster than building from GRAPPA3E (minutes vs hours)\n";
        return 1;
    }
    
    std::string v1_path = argv[1];
    std::string v2_path = argv[2];
    
    auto start_time = std::chrono::steady_clock::now();
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "🔄 Converting Mag18 V1 → V2 with HEALPix Index\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Load V1 catalog
    std::cout << "📖 Loading V1 catalog: " << v1_path << "\n";
    GaiaMag18Catalog v1_catalog(v1_path);
    
    auto stats = v1_catalog.getStatistics();
    std::cout << "✅ Loaded " << stats.total_stars << " stars (G ≤ " << stats.magnitude_limit << ")\n\n";
    
    // Create V2 writer
    std::cout << "📝 Creating V2 catalog: " << v2_path << "\n";
    Mag18CatalogV2Writer writer(v2_path, 64);  // NSIDE=64
    
    // Convert records
    std::cout << "🔄 Converting records and building HEALPix index...\n";
    
    const size_t CHUNK_SIZE = 1000000;  // Process 1M stars at a time
    size_t processed = 0;
    size_t total = stats.total_stars;
    
    // Scan full sky in small patches to avoid memory issues
    for (int ra_idx = 0; ra_idx < 360; ++ra_idx) {
        double ra_center = ra_idx + 0.5;
        
        for (int dec_idx = 0; dec_idx < 180; dec_idx += 10) {
            double dec_center = dec_idx - 90.0 + 5.0;
            
            // Query 10° patch
            auto stars = v1_catalog.queryCone(ra_center, dec_center, 7.1, 0);
            
            for (const auto& star : stars) {
                Mag18RecordV2 v2_record;
                v2_record.source_id = star.source_id;
                v2_record.ra = star.ra;
                v2_record.dec = star.dec;
                v2_record.g_mag = star.g_mag;
                v2_record.bp_mag = star.bp_mag;
                v2_record.rp_mag = star.rp_mag;
                v2_record.g_mag_error = star.g_mag_error;
                v2_record.bp_mag_error = star.bp_mag_error;
                v2_record.rp_mag_error = star.rp_mag_error;
                v2_record.bp_rp = star.bp_rp;
                v2_record.parallax = star.parallax;
                v2_record.parallax_error = star.parallax_error;
                v2_record.pmra = star.pmra;
                v2_record.pmdec = star.pmdec;
                v2_record.pmra_error = star.pmra_error;
                v2_record.ruwe = star.ruwe;
                v2_record.phot_bp_n_obs = star.phot_bp_n_obs;
                v2_record.phot_rp_n_obs = star.phot_rp_n_obs;
                v2_record.healpix_pixel = computeHEALPixPixel(star.ra, star.dec, 64);
                
                writer.addRecord(v2_record);
                
                processed++;
                if (processed % 100000 == 0) {
                    float progress = 100.0f * processed / total;
                    std::cout << "\rProcessed: " << processed << " / " << total 
                              << " (" << std::fixed << std::setprecision(1) << progress << "%)" << std::flush;
                }
            }
        }
    }
    
    std::cout << "\n\n📊 Finalizing catalog (sorting and indexing)...\n";
    writer.finalize();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    
    std::cout << "\n✅ V2 catalog created successfully!\n";
    std::cout << "   Output: " << v2_path << "\n";
    std::cout << "   Stars: " << processed << "\n";
    std::cout << "   Time: " << duration.count() << " seconds\n";
    
    return 0;
}
