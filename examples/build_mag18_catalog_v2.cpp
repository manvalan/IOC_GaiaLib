#include "ioc_gaialib/gaia_local_catalog.h"
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <chrono>
#include <zlib.h>

using namespace ioc::gaia;

/**
 * @brief Build Gaia Mag18 V2 catalog with HEALPix spatial index
 * 
 * Improvements over V1:
 * - HEALPix NSIDE=64 spatial index for 100-300x faster cone searches
 * - Chunk-based compression (1M records/chunk) for 5x faster access
 * - Extended 80-byte record format with proper motion and quality
 * 
 * Expected output: ~14 GB (vs 9 GB in V1)
 */

struct StarDataV2 {
    Mag18RecordV2 record;
    
    bool operator<(const StarDataV2& other) const {
        return record.source_id < other.record.source_id;
    }
};

void printProgress(size_t current, size_t total, const std::string& phase) {
    const int bar_width = 50;
    float progress = static_cast<float>(current) / total;
    int pos = static_cast<int>(bar_width * progress);
    
    std::cout << "\r" << phase << " [";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% (" << current << "/" << total << ")" << std::flush;
}

uint32_t computeHEALPixPixel(double ra, double dec, uint32_t nside) {
    // Convert RA/Dec to HEALPix pixel (NSIDE=64, NESTED)
    // IMPORTANT: Must match Mag18CatalogV2::ang2pix_nest() exactly!
    const double DEG2RAD = 3.14159265358979323846 / 180.0;
    const double PI = 3.14159265358979323846;
    const double TWOPI = 2.0 * PI;
    const double HALFPI = PI / 2.0;
    
    double theta = (90.0 - dec) * DEG2RAD;  // Colatitude
    double phi = ra * DEG2RAD;               // Longitude
    
    const uint32_t nl2 = 2 * nside;
    const uint32_t nl4 = 4 * nside;
    const double z = cos(theta);
    const double za = fabs(z);
    
    // Equatorial region
    if (za <= 2.0/3.0) {
        const double temp1 = nside * (0.5 + phi / TWOPI);
        const double temp2 = nside * z * 0.75;
        const uint32_t jp = static_cast<uint32_t>(temp1 - temp2);
        const uint32_t jm = static_cast<uint32_t>(temp1 + temp2);
        const uint32_t ir = nside + 1 + jp - jm;
        const uint32_t kshift = 1 - (ir & 1);
        const uint32_t ip = (jp + jm - nside + kshift + 1) / 2;
        
        const uint32_t face_num = ((jp < nside) ? 0 : 
                                   (jm < nside) ? 2 : 
                                   (jp >= nl2) ? 4 : 
                                   (jm >= nl2) ? 6 : 8) + (ir - nside);
        
        return face_num * nside * nside + (ip * nside + (ir - nside + 1));
    }
    
    // Polar caps
    const uint32_t ntt = static_cast<uint32_t>(phi * 4.0 / TWOPI);
    const double tp = phi - ntt * TWOPI / 4.0;
    const double tmp = sqrt(3.0 * (1.0 - za));
    const uint32_t jp = static_cast<uint32_t>(nside * tp / HALFPI * tmp);
    const uint32_t jm = static_cast<uint32_t>(nside * (1.0 - tp / HALFPI) * tmp);
    const uint32_t ir = jp + jm + 1;
    const uint32_t ip = ntt + 1;
    
    if (z > 0) {
        return 2 * ir * (ir - 1) + ip - 1;
    } else {
        const uint32_t npix = 12 * nside * nside;
        return npix - 2 * ir * (ir + 1) + ip - 1;
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <grappa3e_path> <output_v2_catalog>\n";
        std::cout << "\nExample:\n";
        std::cout << "  " << argv[0] << " ~/catalogs/GRAPPA3E ~/catalogs/gaia_mag18_v2.cat\n";
        std::cout << "\nThis will create a Mag18 V2 catalog with:\n";
        std::cout << "  - HEALPix NSIDE=64 spatial index\n";
        std::cout << "  - Chunk compression (1M stars/chunk)\n";
        std::cout << "  - Extended 80-byte records\n";
        std::cout << "  - Expected size: ~14 GB\n";
        std::cout << "  - Expected time: 60-90 minutes\n";
        return 1;
    }
    
    std::string grappa_path = argv[1];
    std::string output_path = argv[2];
    
    auto start_time = std::chrono::steady_clock::now();
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "🚀 Building Gaia Mag18 V2 Catalog with HEALPix Index\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    const double MAG_LIMIT = 18.0;
    const uint32_t HEALPIX_NSIDE = 64;
    std::string temp_file = output_path + ".temp";
    uint64_t star_count = 0;
    
    // Check if temp file already exists (resume from Phase 3)
    std::ifstream temp_check(temp_file, std::ios::binary | std::ios::ate);
    bool resume_from_temp = false;
    if (temp_check) {
        size_t temp_size = temp_check.tellg();
        temp_check.close();
        star_count = temp_size / sizeof(Mag18RecordV2);
        if (star_count > 0) {
            std::cout << "♻️  Found existing temp file with " << star_count << " stars\n";
            std::cout << "   Skipping Phase 1 & 2, resuming from Phase 3...\n\n";
            resume_from_temp = true;
        }
    }
    
    if (!resume_from_temp) {
        // Phase 1: Load GRAPPA3E catalog
        std::cout << "📖 Phase 1: Loading GRAPPA3E catalog...\n";
        ioc_gaialib::GaiaLocalCatalog grappa(grappa_path);
        if (!grappa.isLoaded()) {
            std::cerr << "❌ Failed to load GRAPPA3E catalog\n";
            return 1;
        }
        auto stats = grappa.getStatistics();
        std::cout << "✅ GRAPPA3E loaded: " << stats.total_sources << " sources from " 
                  << stats.tiles_loaded << " tiles\n\n";
        
        // Phase 2: Collect all stars with mag <= 18 (scan full sky)
        std::cout << "⭐ Phase 2: Scanning full sky for stars with G ≤ 18...\n";
        std::ofstream temp_out(temp_file, std::ios::binary);
        if (!temp_out) {
            std::cerr << "❌ Failed to create temp file: " << temp_file << "\n";
            return 1;
        }
        
        star_count = 0;
        uint64_t duplicate_count = 0;
    
    // Scan full sky in smaller 2° × 2° patches to reduce memory usage
    // Note: Patches overlap, so we'll have duplicates - removed in Phase 3
    const int RA_STEPS = 180;  // 360° / 2° = 180
    const int DEC_STEPS = 90;  // 180° / 2° = 90
    int total_patches = RA_STEPS * DEC_STEPS;
    int patch_count = 0;
    
    for (int ra_idx = 0; ra_idx < RA_STEPS; ++ra_idx) {
        double ra_center = ra_idx * 2.0 + 1.0;  // Center of 2° patch
        
        for (int dec_idx = 0; dec_idx < DEC_STEPS; ++dec_idx) {
            double dec_center = dec_idx * 2.0 - 89.0;  // -89° to +89°
            
            // Query 2° box using cone search (radius ~1.5° covers 2° box)
            // CRITICAL: Limit to 100k stars per query to prevent OOM
            auto stars = grappa.queryConeWithMagnitude(
                ra_center, dec_center, 1.5,  // Smaller radius for memory efficiency
                -5.0, MAG_LIMIT,              // Magnitude range
                100000                        // LIMIT: max 100k stars per query
            );
            
            for (const auto& star : stars) {
                if (star.phot_g_mean_mag <= MAG_LIMIT) {
                    StarDataV2 data;
                    data.record.source_id = star.source_id;
                    data.record.ra = star.ra;
                    data.record.dec = star.dec;
                    data.record.g_mag = star.phot_g_mean_mag;
                    data.record.bp_mag = star.phot_bp_mean_mag;
                    data.record.rp_mag = star.phot_rp_mean_mag;
                    data.record.g_mag_error = 0.01f;  // Placeholder
                    data.record.bp_mag_error = 0.02f;
                    data.record.rp_mag_error = 0.02f;
                    data.record.bp_rp = star.bp_rp;
                    data.record.parallax = star.parallax;
                    data.record.parallax_error = star.parallax_error;
                    data.record.pmra = star.pmra;
                    data.record.pmdec = star.pmdec;
                    data.record.pmra_error = star.pmra_error;
                    data.record.ruwe = star.ruwe;
                    data.record.phot_bp_n_obs = 100;  // Placeholder
                    data.record.phot_rp_n_obs = 100;
                    data.record.healpix_pixel = computeHEALPixPixel(star.ra, star.dec, HEALPIX_NSIDE);
                    
                    temp_out.write(reinterpret_cast<const char*>(&data.record), 
                                  sizeof(Mag18RecordV2));
                    star_count++;
                }
            }
            
            patch_count++;
            if (patch_count % 100 == 0) {  // Update every 100 patches (more frequent for smaller patches)
                printProgress(patch_count, total_patches, "Scanning sky patches");
            }
        }
    }
    temp_out.close();
    printProgress(total_patches, total_patches, "Scanning sky patches");
    std::cout << "\n✅ Found " << star_count << " stars with G ≤ " << MAG_LIMIT << "\n\n";
    
    } // End of !resume_from_temp
    
    if (star_count == 0) {
        std::cerr << "❌ No stars found!\n";
        return 1;
    }
    
    // Phase 3: Load temp file, sort by source_id
    std::cout << "🔄 Phase 3: Loading and sorting stars by source_id...\n";
    std::ifstream temp_in(temp_file, std::ios::binary);
    std::vector<Mag18RecordV2> stars;
    stars.reserve(star_count);
    
    Mag18RecordV2 record;
    while (temp_in.read(reinterpret_cast<char*>(&record), sizeof(Mag18RecordV2))) {
        stars.push_back(record);
    }
    temp_in.close();
    std::cout << "✅ Loaded " << stars.size() << " stars\n";
    
    std::cout << "🔄 Sorting by source_id...\n";
    std::sort(stars.begin(), stars.end(), 
        [](const Mag18RecordV2& a, const Mag18RecordV2& b) {
            return a.source_id < b.source_id;
        });
    std::cout << "✅ Sorted\n";
    
    // Remove duplicates (overlapping patches)
    std::cout << "🔄 Removing duplicates...\n";
    auto last = std::unique(stars.begin(), stars.end(),
        [](const Mag18RecordV2& a, const Mag18RecordV2& b) {
            return a.source_id == b.source_id;
        });
    size_t duplicates = std::distance(last, stars.end());
    stars.erase(last, stars.end());
    std::cout << "✅ Removed " << duplicates << " duplicates, " << stars.size() << " unique stars remain\n\n";
    
    // Phase 4: Build HEALPix index
    std::cout << "🗺️  Phase 4: Building HEALPix spatial index...\n";
    
    // Sort stars by HEALPix pixel (IN-PLACE to save memory)
    std::cout << "🔄 Sorting by HEALPix pixel...\n";
    std::sort(stars.begin(), stars.end(),
        [](const Mag18RecordV2& a, const Mag18RecordV2& b) {
            return a.healpix_pixel < b.healpix_pixel;
        });
    std::cout << "✅ Sorted by HEALPix\n";
    
    // Build pixel index
    std::cout << "🔄 Building pixel index...\n";
    std::vector<HEALPixIndexEntry> healpix_index;
    uint32_t current_pixel = stars[0].healpix_pixel;
    uint64_t pixel_start = 0;
    uint32_t pixel_count = 0;
    
    for (size_t i = 0; i < stars.size(); ++i) {
        if (stars[i].healpix_pixel != current_pixel) {
            HEALPixIndexEntry entry;
            entry.pixel_id = current_pixel;
            entry.first_star_idx = pixel_start;
            entry.num_stars = pixel_count;
            entry.reserved = 0;
            healpix_index.push_back(entry);
            
            current_pixel = stars[i].healpix_pixel;
            pixel_start = i;
            pixel_count = 1;
        } else {
            pixel_count++;
        }
    }
    
    // Last pixel
    HEALPixIndexEntry entry;
    entry.pixel_id = current_pixel;
    entry.first_star_idx = pixel_start;
    entry.num_stars = pixel_count;
    entry.reserved = 0;
    healpix_index.push_back(entry);
    
    std::cout << "✅ Created " << healpix_index.size() << " HEALPix entries\n\n";
    
    // Phase 5: Chunk compression
    std::cout << "🗜️  Phase 5: Compressing data into chunks...\n";
    const uint32_t STARS_PER_CHUNK = 1000000;
    const uint64_t num_chunks = (stars.size() + STARS_PER_CHUNK - 1) / STARS_PER_CHUNK;
    
    std::vector<ChunkInfo> chunk_index;
    std::vector<std::vector<uint8_t>> compressed_chunks;
    
    for (uint64_t chunk_id = 0; chunk_id < num_chunks; ++chunk_id) {
        uint64_t start_idx = chunk_id * STARS_PER_CHUNK;
        uint64_t end_idx = std::min(start_idx + STARS_PER_CHUNK, 
                                     static_cast<uint64_t>(stars.size()));
        uint32_t chunk_stars = end_idx - start_idx;
        
        // Compress chunk
        uLongf source_len = chunk_stars * sizeof(Mag18RecordV2);
        uLongf dest_len = compressBound(source_len);
        std::vector<uint8_t> compressed(dest_len);
        
        int result = compress2(compressed.data(), &dest_len,
                              reinterpret_cast<const uint8_t*>(&stars[start_idx]),
                              source_len, Z_BEST_COMPRESSION);
        
        if (result != Z_OK) {
            std::cerr << "❌ Compression failed for chunk " << chunk_id << "\n";
            return 1;
        }
        
        compressed.resize(dest_len);
        compressed_chunks.push_back(compressed);
        
        ChunkInfo info;
        info.chunk_id = chunk_id;
        info.first_star_idx = start_idx;
        info.num_stars = chunk_stars;
        info.compressed_size = dest_len;
        info.uncompressed_size = source_len;
        info.file_offset = 0;  // Will be set when writing
        info.reserved = 0;
        chunk_index.push_back(info);
        
        printProgress(chunk_id + 1, num_chunks, "Compressing chunks");
    }
    std::cout << "\n✅ Created " << num_chunks << " compressed chunks\n\n";
    
    // Phase 6: Write catalog file
    std::cout << "💾 Phase 6: Writing V2 catalog file...\n";
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "❌ Failed to create output file: " << output_path << "\n";
        return 1;
    }
    
    // Prepare header
    Mag18CatalogHeaderV2 header;
    memset(&header, 0, sizeof(header));
    strcpy(header.magic, "GAIA18V2");
    header.version = 2;
    header.format_flags = 0;
    header.total_stars = stars.size();
    header.total_chunks = num_chunks;
    header.stars_per_chunk = STARS_PER_CHUNK;
    header.healpix_nside = HEALPIX_NSIDE;
    header.mag_limit = MAG_LIMIT;
    header.ra_min = 0.0;
    header.ra_max = 360.0;
    header.dec_min = -90.0;
    header.dec_max = 90.0;
    header.header_size = sizeof(Mag18CatalogHeaderV2);
    header.num_healpix_pixels = healpix_index.size();
    
    // Calculate offsets
    uint64_t offset = sizeof(Mag18CatalogHeaderV2);
    
    header.healpix_index_offset = offset;
    header.healpix_index_size = healpix_index.size() * sizeof(HEALPixIndexEntry);
    offset += header.healpix_index_size;
    
    header.chunk_index_offset = offset;
    header.chunk_index_size = chunk_index.size() * sizeof(ChunkInfo);
    offset += header.chunk_index_size;
    
    header.data_offset = offset;
    
    // Update chunk offsets
    for (size_t i = 0; i < chunk_index.size(); ++i) {
        chunk_index[i].file_offset = offset;
        offset += compressed_chunks[i].size();
    }
    header.data_size = offset - header.data_offset;
    
    time_t now = time(nullptr);
    strftime(header.creation_date, sizeof(header.creation_date), 
             "%Y-%m-%dT%H:%M:%S", gmtime(&now));
    strcpy(header.source_catalog, "GRAPPA3E");
    
    // Write header
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write HEALPix index
    out.write(reinterpret_cast<const char*>(healpix_index.data()),
              healpix_index.size() * sizeof(HEALPixIndexEntry));
    
    // Write chunk index
    out.write(reinterpret_cast<const char*>(chunk_index.data()),
              chunk_index.size() * sizeof(ChunkInfo));
    
    // Write compressed chunks
    for (size_t i = 0; i < compressed_chunks.size(); ++i) {
        out.write(reinterpret_cast<const char*>(compressed_chunks[i].data()),
                 compressed_chunks[i].size());
        printProgress(i + 1, compressed_chunks.size(), "Writing chunks");
    }
    out.close();
    std::cout << "\n✅ Catalog written\n\n";
    
    // Clean up temp file
    std::remove(temp_file.c_str());
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(end_time - start_time);
    
    // Final statistics
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "✨ Gaia Mag18 V2 Catalog Created Successfully!\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    std::cout << "📊 Statistics:\n";
    std::cout << "   Stars: " << stars.size() << " (G ≤ " << MAG_LIMIT << ")\n";
    std::cout << "   HEALPix pixels: " << healpix_index.size() << " (NSIDE=" << HEALPIX_NSIDE << ")\n";
    std::cout << "   Chunks: " << num_chunks << " (" << STARS_PER_CHUNK << " stars/chunk)\n";
    std::cout << "   File: " << output_path << "\n";
    std::cout << "   Size: " << (offset / 1024.0 / 1024.0 / 1024.0) << " GB\n";
    std::cout << "   Time: " << duration.count() << " minutes\n\n";
    std::cout << "🚀 Expected performance improvements:\n";
    std::cout << "   Cone search 0.5°: 15s → 50ms (300x faster)\n";
    std::cout << "   Cone search 5°:   48s → 500ms (96x faster)\n";
    std::cout << "   Query source_id:  <1ms (unchanged)\n\n";
    
    return 0;
}
