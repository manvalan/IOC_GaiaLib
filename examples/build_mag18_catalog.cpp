#include "ioc_gaialib/gaia_local_catalog.h"
#include "ioc_gaialib/gaia_mag18_catalog.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstring>
#include <zlib.h>
#include <iomanip>

namespace fs = std::filesystem;
using namespace ioc_gaialib;
using namespace ioc::gaia;

void printProgress(size_t current, size_t total, const std::string& label) {
    if (total == 0) return;
    
    int percent = (current * 100) / total;
    int bar_width = 50;
    int filled = (current * bar_width) / total;
    
    std::cout << "\r" << label << " [";
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) std::cout << "=";
        else if (i == filled) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << percent << "% (" << current << "/" << total << ")";
    std::cout.flush();
}

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  GRAPPA3E → Gaia Mag18 Compressed Catalog Builder         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <grappa3e_path> <output_file.gz>\n";
        std::cerr << "\nExample:\n";
        std::cerr << "  " << argv[0] << " ~/catalogs/GRAPPA3E gaia_mag18.cat.gz\n\n";
        return 1;
    }
    
    std::string grappa_path = argv[1];
    std::string output_file = argv[2];
    
    try {
        // ====================================================================
        // PHASE 1: Initialize catalog
        // ====================================================================
        std::cout << "📂 Phase 1: Loading GRAPPA3E catalog...\n" << std::endl;
        GaiaLocalCatalog catalog(grappa_path);
        
        if (!catalog.isLoaded()) {
            std::cerr << "❌ Failed to load catalog from: " << grappa_path << std::endl;
            return 1;
        }
        
        std::cout << "✅ Catalog loaded: " << catalog.getCatalogPath() << "\n" << std::endl;
        
        // ====================================================================
        // PHASE 2: Scan tiles and write to temporary file (memory-efficient)
        // ====================================================================
        std::cout << "🔍 Phase 2: Scanning tiles for mag ≤ 18 stars...\n" << std::endl;
        
        const double MAG_LIMIT = 18.0;
        std::string temp_file = output_file + ".tmp";
        std::ofstream temp_out(temp_file, std::ios::binary);
        
        if (!temp_out) {
            std::cerr << "❌ Failed to create temporary file: " << temp_file << std::endl;
            return 1;
        }
        
        size_t total_stars = 0;
        size_t tiles_processed = 0;
        size_t total_tiles = 170 * 360 + 2;  // 61,202 tiles
        
        // Scan all Dec bands
        for (int dec_band = 5; dec_band <= 174; ++dec_band) {
            for (int ra_band = 0; ra_band < 360; ++ra_band) {
                // Query small box for this tile
                double ra_min = ra_band;
                double ra_max = ra_band + 1.0;
                double dec_min = dec_band - 90.0;
                double dec_max = dec_band - 89.0;
                
                auto tile_stars = catalog.queryBox(ra_min, ra_max, dec_min, dec_max, 0);
                
                // Write stars to temp file immediately
                for (const auto& star : tile_stars) {
                    if (star.phot_g_mean_mag > 0 && star.phot_g_mean_mag <= MAG_LIMIT) {
                        Mag18Record record;
                        record.source_id = star.source_id;
                        record.ra = star.ra;
                        record.dec = star.dec;
                        record.g_mag = star.phot_g_mean_mag;
                        record.bp_mag = star.phot_bp_mean_mag;
                        record.rp_mag = star.phot_rp_mean_mag;
                        record.parallax = static_cast<float>(star.parallax);
                        
                        temp_out.write(reinterpret_cast<const char*>(&record), sizeof(record));
                        total_stars++;
                    }
                }
                
                tiles_processed++;
                if (tiles_processed % 100 == 0) {
                    printProgress(tiles_processed, total_tiles, "  Scanning");
                }
            }
        }
        
        temp_out.close();
        
        std::cout << "\r✅ Scanning complete: " << total_stars 
                  << " stars with mag ≤ " << MAG_LIMIT << " written to temp file          \n" << std::endl;
        
        // ====================================================================
        // PHASE 3: Load temp file, sort by source_id, and write back
        // ====================================================================
        std::cout << "🔀 Phase 3: Loading and sorting by source_id...\n" << std::endl;
        
        // Read all records from temp file
        std::ifstream temp_in(temp_file, std::ios::binary);
        if (!temp_in) {
            std::cerr << "❌ Failed to open temporary file for reading\n";
            return 1;
        }
        
        std::vector<Mag18Record> records;
        records.reserve(total_stars);
        
        Mag18Record record;
        while (temp_in.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            records.push_back(record);
        }
        temp_in.close();
        
        std::cout << "  Loaded " << records.size() << " records from temp file\n";
        std::cout << "  Sorting...\n";
        
        // Sort by source_id
        std::sort(records.begin(), records.end(),
                  [](const Mag18Record& a, const Mag18Record& b) {
                      return a.source_id < b.source_id;
                  });
        
        std::cout << "✅ Sorted " << records.size() << " stars\n" << std::endl;
        
        // ====================================================================
        // PHASE 4: Write sorted records back to temp file
        // ====================================================================
        std::cout << "📦 Phase 4: Writing sorted data...\n" << std::endl;
        
        std::ofstream sorted_out(temp_file, std::ios::binary | std::ios::trunc);
        if (!sorted_out) {
            std::cerr << "❌ Failed to write sorted data\n";
            return 1;
        }
        
        for (size_t i = 0; i < records.size(); ++i) {
            sorted_out.write(reinterpret_cast<const char*>(&records[i]), sizeof(Mag18Record));
            
            if ((i + 1) % 1000000 == 0) {
                printProgress(i + 1, records.size(), "  Writing");
            }
        }
        sorted_out.close();
        
        size_t binary_size = records.size() * sizeof(Mag18Record);
        
        std::cout << "\r✅ Written " << records.size() 
                  << " sorted records          \n" << std::endl;
        
        // Free memory
        records.clear();
        records.shrink_to_fit();
        
        // ====================================================================
        // PHASE 5: Compress with gzip
        // ====================================================================
        std::cout << "🗜️  Phase 5: Compressing data...\n" << std::endl;
        
        // Open gzip file
        gzFile gz = gzopen(output_file.c_str(), "wb9");  // Max compression
        if (!gz) {
            std::cerr << "❌ Failed to create output file: " << output_file << std::endl;
            return 1;
        }
        
        // Write header
        Mag18CatalogHeader header{};
        std::strcpy(header.magic, "GAIA18");
        header.version = 1;
        header.total_stars = total_stars;
        header.mag_limit = MAG_LIMIT;
        header.data_offset = sizeof(Mag18CatalogHeader);
        header.data_size = binary_size;
        
        gzwrite(gz, &header, sizeof(Mag18CatalogHeader));
        
        // Read temp file and write compressed in chunks
        std::ifstream final_in(temp_file, std::ios::binary);
        if (!final_in) {
            std::cerr << "❌ Failed to open temp file for compression\n";
            gzclose(gz);
            return 1;
        }
        
        const size_t chunk_size = 1024 * 1024;  // 1 MB chunks
        std::vector<char> buffer(chunk_size);
        size_t total_written = 0;
        
        while (final_in) {
            final_in.read(buffer.data(), chunk_size);
            std::streamsize bytes_read = final_in.gcount();
            
            if (bytes_read > 0) {
                gzwrite(gz, buffer.data(), bytes_read);
                total_written += bytes_read;
                printProgress(total_written, binary_size, "  Compressing");
            }
        }
        
        final_in.close();
        gzclose(gz);
        
        // Clean up temp file
        fs::remove(temp_file);
        
        std::cout << "\r✅ Compression complete                              \n" << std::endl;
        
        // ====================================================================
        // SUMMARY
        // ====================================================================
        size_t compressed_size = fs::file_size(output_file);
        double compression_ratio = 100.0 * (1.0 - (double)compressed_size / binary_size);
        
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  CATALOG CREATION COMPLETE                                 ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "📊 Statistics:\n";
        std::cout << "  Stars included:     " << total_stars << "\n";
        std::cout << "  Magnitude limit:    ≤ " << MAG_LIMIT << "\n";
        std::cout << "  Uncompressed size:  " << (binary_size / (1024*1024)) << " MB\n";
        std::cout << "  Compressed size:    " << (compressed_size / (1024*1024)) << " MB\n";
        std::cout << "  Compression ratio:  " << std::fixed << std::setprecision(1) 
                  << compression_ratio << "%\n";
        std::cout << "  Output file:        " << output_file << "\n";
        std::cout << "\n✅ Ready to use with GaiaMag18Catalog class!\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
