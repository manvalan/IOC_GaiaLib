#include <iostream>
#include <fstream>
#include "../include/ioc_gaialib/gaia_mag18_catalog_v2.h"
#include "../include/ioc_gaialib/multifile_catalog_v2.h"

using namespace ioc::gaia;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <multifile_catalog_directory>" << std::endl;
        return 1;
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ Raw Chunk Data Test                                       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝\n" << std::endl;

    std::string catalog_dir = argv[1];
    
    // Test: directly read first chunk file
    std::string chunk_path = catalog_dir + "/chunks/chunk_000.dat";
    std::ifstream chunk_file(chunk_path, std::ios::binary);
    
    if (!chunk_file) {
        std::cerr << "❌ Cannot open chunk file: " << chunk_path << std::endl;
        return 1;
    }
    
    // Get file size
    chunk_file.seekg(0, std::ios::end);
    size_t file_size = chunk_file.tellg();
    chunk_file.seekg(0, std::ios::beg);
    
    std::cout << "📁 Chunk file: " << chunk_path << std::endl;
    std::cout << "📏 File size: " << file_size << " bytes" << std::endl;
    
    // Read a few records directly
    const size_t record_size = sizeof(Mag18RecordV2);
    const size_t num_records = file_size / record_size;
    
    std::cout << "📊 Record size: " << record_size << " bytes" << std::endl;
    std::cout << "📊 Expected records: " << num_records << std::endl;
    
    // Read first 10 records
    for (size_t i = 0; i < std::min<size_t>(10, num_records); i++) {
        Mag18RecordV2 record;
        chunk_file.read(reinterpret_cast<char*>(&record), sizeof(record));
        
        if (chunk_file.good()) {
            std::cout << "⭐ Record " << i << ": "
                      << "source_id=" << record.source_id 
                      << ", RA=" << record.ra 
                      << "°, Dec=" << record.dec 
                      << "°, Mag=" << record.g_mag 
                      << ", HEALPix=" << record.healpix_pixel << std::endl;
        } else {
            std::cout << "❌ Error reading record " << i << std::endl;
            break;
        }
    }
    
    chunk_file.close();
    
    // Test metadata file
    std::cout << "\n🗂️  Testing metadata file:" << std::endl;
    std::string metadata_path = catalog_dir + "/metadata.dat";
    std::ifstream metadata_file(metadata_path, std::ios::binary);
    
    if (!metadata_file) {
        std::cerr << "❌ Cannot open metadata file: " << metadata_path << std::endl;
        return 1;
    }
    
    // Read header
    Mag18CatalogHeaderV2 header;
    metadata_file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (metadata_file.good()) {
        std::cout << "📋 Header loaded successfully:" << std::endl;
        std::cout << "   Magic: " << std::string(header.magic, 8) << std::endl;
        std::cout << "   Version: " << header.version << std::endl;
        std::cout << "   Total stars: " << header.total_stars << std::endl;
        std::cout << "   Total chunks: " << header.total_chunks << std::endl;
        std::cout << "   Stars per chunk: " << header.stars_per_chunk << std::endl;
        std::cout << "   HEALPix NSIDE: " << header.healpix_nside << std::endl;
        std::cout << "   HEALPix pixels: " << header.num_healpix_pixels << std::endl;
        std::cout << "   Magnitude limit: " << header.mag_limit << std::endl;
        
        // Read pixel index
        std::cout << "\n🎯 Reading HEALPix index..." << std::endl;
        for (uint32_t i = 0; i < std::min<uint32_t>(10, header.num_healpix_pixels); i++) {
            HEALPixIndexEntry entry;
            metadata_file.read(reinterpret_cast<char*>(&entry), sizeof(entry));
            
            if (metadata_file.good()) {
                std::cout << "   Pixel " << i << ": id=" << entry.pixel_id 
                          << ", first_star=" << entry.first_star_idx 
                          << ", num_stars=" << entry.num_stars << std::endl;
            } else {
                std::cout << "❌ Error reading pixel entry " << i << std::endl;
                break;
            }
        }
    } else {
        std::cout << "❌ Error reading header" << std::endl;
    }
    
    metadata_file.close();
    
    return 0;
}