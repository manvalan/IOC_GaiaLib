// Low-Level I/O Test - Check catalog file structure
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cstdint>

#pragma pack(push, 1)
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

struct HEALPixIndexEntry {
    uint32_t pixel_id;
    uint64_t first_star_idx;
    uint32_t num_stars;
    uint32_t reserved;
};
#pragma pack(pop)

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog.mag18v2>\n";
        return 1;
    }
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  V2 Catalog I/O Test                                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        std::cerr << "❌ Cannot open file: " << argv[1] << "\n";
        return 1;
    }
    
    std::cout << "✅ File opened\n\n";
    
    // Read header
    std::cout << "Reading header (256 bytes)...\n";
    Mag18CatalogHeaderV2 header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        std::cerr << "❌ Failed to read header\n";
        fclose(file);
        return 1;
    }
    std::cout << "✅ Header read\n\n";
    
    // Validate header
    std::cout << "Validating header:\n";
    std::cout << "  Magic: " << std::string(header.magic, 8);
    if (std::strncmp(header.magic, "GAIA18V2", 8) != 0) {
        std::cout << " ❌ INVALID\n";
        fclose(file);
        return 1;
    }
    std::cout << " ✅\n";
    
    std::cout << "  Version: " << header.version;
    if (header.version != 2) {
        std::cout << " ❌ INVALID (expected 2)\n";
        fclose(file);
        return 1;
    }
    std::cout << " ✅\n\n";
    
    // Print statistics
    std::cout << "Catalog Statistics:\n";
    std::cout << "  Total stars: " << header.total_stars << "\n";
    std::cout << "  Total chunks: " << header.total_chunks << "\n";
    std::cout << "  Stars per chunk: " << header.stars_per_chunk << "\n";
    std::cout << "  HEALPix NSIDE: " << header.healpix_nside << "\n";
    std::cout << "  HEALPix pixels with data: " << header.num_healpix_pixels << "\n";
    std::cout << "  Magnitude limit: ≤ " << std::fixed << std::setprecision(1) 
              << header.mag_limit << "\n";
    std::cout << "  RA range: " << header.ra_min << "° to " << header.ra_max << "°\n";
    std::cout << "  Dec range: " << header.dec_min << "° to " << header.dec_max << "°\n";
    std::cout << "  Creation date: " << std::string(header.creation_date, 32) << "\n";
    std::cout << "  Source: " << std::string(header.source_catalog, 32) << "\n\n";
    
    // File offsets
    std::cout << "File Layout:\n";
    std::cout << "  Header offset: 0\n";
    std::cout << "  HEALPix index offset: " << header.healpix_index_offset 
              << " (size: " << header.healpix_index_size << " bytes)\n";
    std::cout << "  Chunk index offset: " << header.chunk_index_offset 
              << " (size: " << header.chunk_index_size << " bytes)\n";
    std::cout << "  Data offset: " << header.data_offset 
              << " (size: " << header.data_size << " bytes)\n\n";
    
    // Read and validate HEALPix index
    std::cout << "Reading HEALPix index (" << header.num_healpix_pixels 
              << " entries)...\n";
    if (fseek(file, header.healpix_index_offset, SEEK_SET) != 0) {
        std::cerr << "❌ Seek to HEALPix index failed\n";
        fclose(file);
        return 1;
    }
    
    std::vector<HEALPixIndexEntry> healpix_entries(header.num_healpix_pixels);
    if (fread(healpix_entries.data(), sizeof(HEALPixIndexEntry), 
              header.num_healpix_pixels, file) != header.num_healpix_pixels) {
        std::cerr << "❌ Failed to read HEALPix index\n";
        fclose(file);
        return 1;
    }
    std::cout << "✅ HEALPix index read\n\n";
    
    // Sample pixels
    std::cout << "Sample HEALPix pixels:\n";
    for (size_t i = 0; i < std::min(size_t(5), healpix_entries.size()); ++i) {
        std::cout << "  Pixel " << i << ": ID=" << healpix_entries[i].pixel_id 
                  << " Stars=" << healpix_entries[i].num_stars 
                  << " (idx " << healpix_entries[i].first_star_idx << ")\n";
    }
    std::cout << "\n";
    
    // Check data can be read
    std::cout << "Checking data access...\n";
    if (fseek(file, header.data_offset, SEEK_SET) != 0) {
        std::cerr << "❌ Seek to data offset failed\n";
        fclose(file);
        return 1;
    }
    
    uint8_t buffer[256];
    if (fread(buffer, 1, 256, file) != 256) {
        std::cerr << "❌ Failed to read data\n";
        fclose(file);
        return 1;
    }
    std::cout << "✅ Data readable\n\n";
    
    // Summary
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ I/O TEST PASSED - File structure is valid              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    fclose(file);
    return 0;
}
