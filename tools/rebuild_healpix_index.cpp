/**
 * @file rebuild_healpix_index.cpp
 * @brief Rebuilds HEALPix spatial index for multifile catalog
 * 
 * This tool scans all chunks and creates a correct HEALPix index
 * that maps each pixel to the chunks containing stars in that pixel.
 * 
 * Usage: rebuild_healpix_index <catalog_dir>
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <algorithm>

// Record structure (must match Mag18RecordV2)
#pragma pack(push, 4)
struct Mag18RecordV2 {
    uint64_t source_id;
    double ra;
    double dec;
    float g_mag, bp_mag, rp_mag;
    float g_mag_error, bp_mag_error, rp_mag_error;
    float bp_rp;
    float parallax, parallax_error;
    float pmra, pmdec, pmra_error;
    float ruwe;
    uint16_t phot_bp_n_obs, phot_rp_n_obs;
    uint32_t healpix_pixel;
};

struct Mag18CatalogHeaderV2 {
    char magic[8];
    uint32_t version;
    uint32_t format_flags;
    uint64_t total_stars;
    uint64_t total_chunks;
    uint32_t stars_per_chunk;
    uint32_t healpix_nside;
    double mag_limit;
    double ra_min, ra_max;
    double dec_min, dec_max;
    uint64_t header_size;
    uint64_t healpix_index_offset;
    uint64_t healpix_index_size;
    uint32_t num_healpix_pixels;
    uint64_t chunk_index_offset;
    uint64_t chunk_index_size;
    uint64_t data_offset;
    uint64_t data_size;
    char creation_date[32];
    char source_catalog[32];
    char reserved[64];
};

// New index entry: maps pixel to chunks containing stars in that pixel
struct PixelChunkEntry {
    uint32_t pixel_id;
    uint32_t num_chunks;      // Number of chunks with stars in this pixel
    uint64_t chunk_list_offset;  // Offset into chunk list array
};

struct ChunkPixelInfo {
    uint32_t chunk_id;
    uint32_t first_star_offset;  // Offset of first star of this pixel in chunk
    uint32_t num_stars;          // Number of stars of this pixel in chunk
};
#pragma pack(pop)

static_assert(sizeof(Mag18RecordV2) == 84, "Mag18RecordV2 must be 84 bytes");

const uint32_t NSIDE = 64;  // HEALPix NSIDE
const uint32_t NPIX = 12 * NSIDE * NSIDE;  // 49152 pixels

// HEALPix ang2pix_nest implementation
uint32_t ang2pix_nest(double theta, double phi) {
    const double z = cos(theta);
    const double za = fabs(z);
    const double TWOPI = 2.0 * M_PI;
    const double HALFPI = M_PI / 2.0;
    
    if (za <= 2.0/3.0) {
        // Equatorial region
        const double temp1 = NSIDE * (0.5 + phi / TWOPI);
        const double temp2 = NSIDE * z * 0.75;
        const int32_t jp = static_cast<int32_t>(temp1 - temp2);
        const int32_t jm = static_cast<int32_t>(temp1 + temp2);
        const int32_t ir = NSIDE + 1 + jp - jm;
        const int32_t kshift = 1 - (ir & 1);
        const int32_t ip = (jp + jm - NSIDE + kshift + 1) / 2;
        const int32_t iphi = ip % (4 * NSIDE);
        
        return (ir - 1) * 4 * NSIDE + iphi;
    }
    
    // Polar caps
    const double tp = phi / HALFPI;
    const double tmp = NSIDE * sqrt(3.0 * (1.0 - za));
    int32_t jp = static_cast<int32_t>(tp * tmp);
    int32_t jm = static_cast<int32_t>((1.0 - tp) * tmp);
    
    if (jp >= NSIDE) jp = NSIDE - 1;
    if (jm >= NSIDE) jm = NSIDE - 1;
    
    if (z > 0) {
        // North polar cap
        const int32_t face = static_cast<int32_t>(phi * 2.0 / M_PI);
        return face * NSIDE * NSIDE + jp * NSIDE + jm;
    } else {
        // South polar cap
        const int32_t face = static_cast<int32_t>(phi * 2.0 / M_PI) + 8;
        return face * NSIDE * NSIDE + jp * NSIDE + jm;
    }
}

uint32_t getHEALPixPixel(double ra, double dec) {
    double theta = (90.0 - dec) * M_PI / 180.0;  // Colatitude
    double phi = ra * M_PI / 180.0;               // Longitude
    if (phi < 0) phi += 2 * M_PI;
    if (phi >= 2 * M_PI) phi -= 2 * M_PI;
    return ang2pix_nest(theta, phi);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog_directory>\n";
        std::cerr << "Example: " << argv[0] << " ~/.catalog/gaia_mag18_v2_multifile\n";
        return 1;
    }
    
    std::string catalog_dir = argv[1];
    std::string metadata_path = catalog_dir + "/metadata.dat";
    std::string chunks_dir = catalog_dir + "/chunks";
    
    std::cout << "=== HEALPix Index Rebuilder ===\n\n";
    std::cout << "Catalog: " << catalog_dir << "\n";
    
    // Read existing header
    std::ifstream meta_in(metadata_path, std::ios::binary);
    if (!meta_in) {
        std::cerr << "Cannot open metadata file: " << metadata_path << "\n";
        return 1;
    }
    
    Mag18CatalogHeaderV2 header;
    meta_in.read(reinterpret_cast<char*>(&header), sizeof(header));
    meta_in.close();
    
    std::cout << "Total stars: " << header.total_stars << "\n";
    std::cout << "Total chunks: " << header.total_chunks << "\n";
    std::cout << "HEALPix NSIDE: " << header.healpix_nside << "\n";
    std::cout << "Max pixels: " << NPIX << "\n\n";
    
    // Map: pixel_id -> set of chunk_ids that contain stars in that pixel
    std::map<uint32_t, std::set<uint32_t>> pixel_to_chunks;
    
    // Detailed map: (pixel_id, chunk_id) -> count of stars
    std::map<std::pair<uint32_t, uint32_t>, uint32_t> pixel_chunk_counts;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Scan all chunks
    uint64_t total_processed = 0;
    for (uint32_t chunk_id = 0; chunk_id < header.total_chunks; ++chunk_id) {
        char chunk_name[32];
        snprintf(chunk_name, sizeof(chunk_name), "/chunk_%03u.dat", chunk_id);
        std::string chunk_path = chunks_dir + chunk_name;
        
        std::ifstream chunk_file(chunk_path, std::ios::binary);
        if (!chunk_file) {
            std::cerr << "Cannot open chunk: " << chunk_path << "\n";
            continue;
        }
        
        // Get chunk size
        chunk_file.seekg(0, std::ios::end);
        size_t file_size = chunk_file.tellg();
        size_t num_records = file_size / sizeof(Mag18RecordV2);
        chunk_file.seekg(0);
        
        // Read all records
        std::vector<Mag18RecordV2> records(num_records);
        chunk_file.read(reinterpret_cast<char*>(records.data()), file_size);
        
        // Process each record
        for (const auto& record : records) {
            uint32_t pixel = getHEALPixPixel(record.ra, record.dec);
            if (pixel < NPIX) {
                pixel_to_chunks[pixel].insert(chunk_id);
                pixel_chunk_counts[{pixel, chunk_id}]++;
            }
        }
        
        total_processed += num_records;
        
        if ((chunk_id + 1) % 20 == 0 || chunk_id == header.total_chunks - 1) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            double progress = 100.0 * (chunk_id + 1) / header.total_chunks;
            std::cout << "\rProcessed chunk " << (chunk_id + 1) << "/" << header.total_chunks 
                      << " (" << std::fixed << std::setprecision(1) << progress << "%) "
                      << total_processed << " stars, " << elapsed << "s" << std::flush;
        }
    }
    
    std::cout << "\n\nBuilding new index...\n";
    
    // Statistics
    uint32_t pixels_with_data = pixel_to_chunks.size();
    uint32_t max_chunks_per_pixel = 0;
    uint64_t total_entries = 0;
    
    for (const auto& [pixel, chunks] : pixel_to_chunks) {
        max_chunks_per_pixel = std::max(max_chunks_per_pixel, static_cast<uint32_t>(chunks.size()));
        total_entries += chunks.size();
    }
    
    std::cout << "Pixels with data: " << pixels_with_data << " / " << NPIX << "\n";
    std::cout << "Max chunks per pixel: " << max_chunks_per_pixel << "\n";
    std::cout << "Total index entries: " << total_entries << "\n\n";
    
    // Build compact index structure
    // Format: 
    //   1. Array of PixelChunkEntry (one per pixel with data)
    //   2. Array of chunk_ids (variable length per pixel)
    
    std::vector<PixelChunkEntry> pixel_index;
    std::vector<uint32_t> chunk_lists;
    
    pixel_index.reserve(pixels_with_data);
    chunk_lists.reserve(total_entries);
    
    for (const auto& [pixel, chunks] : pixel_to_chunks) {
        PixelChunkEntry entry;
        entry.pixel_id = pixel;
        entry.num_chunks = chunks.size();
        entry.chunk_list_offset = chunk_lists.size();
        pixel_index.push_back(entry);
        
        for (uint32_t chunk_id : chunks) {
            chunk_lists.push_back(chunk_id);
        }
    }
    
    // Write new metadata file
    std::string new_metadata_path = catalog_dir + "/metadata_new.dat";
    std::ofstream meta_out(new_metadata_path, std::ios::binary);
    
    // Update header
    header.num_healpix_pixels = pixels_with_data;
    header.healpix_index_offset = sizeof(Mag18CatalogHeaderV2);
    header.healpix_index_size = pixel_index.size() * sizeof(PixelChunkEntry) + 
                                chunk_lists.size() * sizeof(uint32_t);
    
    // Get current time for creation date
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::strftime(header.creation_date, sizeof(header.creation_date), 
                  "%Y-%m-%dT%H:%M:%S", std::localtime(&time_t_now));
    
    // Write header
    meta_out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write pixel index
    meta_out.write(reinterpret_cast<const char*>(pixel_index.data()), 
                   pixel_index.size() * sizeof(PixelChunkEntry));
    
    // Write chunk lists
    meta_out.write(reinterpret_cast<const char*>(chunk_lists.data()),
                   chunk_lists.size() * sizeof(uint32_t));
    
    meta_out.close();
    
    auto end_time = std::chrono::steady_clock::now();
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    
    std::cout << "New index written to: " << new_metadata_path << "\n";
    std::cout << "Index size: " << (header.healpix_index_size / 1024) << " KB\n";
    std::cout << "Total time: " << total_seconds << " seconds\n\n";
    
    std::cout << "To apply the new index, run:\n";
    std::cout << "  mv " << metadata_path << " " << catalog_dir << "/metadata_old.dat\n";
    std::cout << "  mv " << new_metadata_path << " " << metadata_path << "\n";
    
    return 0;
}
