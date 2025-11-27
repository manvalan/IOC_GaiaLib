#include "ioc_gaialib/catalog_compiler.h"
#include "ioc_gaialib/gaia_client.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <thread>
#include <mutex>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <unordered_set>
#include <cstring>
#include <cmath>
#include <zlib.h>

namespace fs = std::filesystem;

namespace ioc {
namespace gaia {

// =============================================================================
// CompactStar Implementation
// =============================================================================

CompactStar CompactStar::fromGaiaStar(const GaiaStar& star) {
    CompactStar compact;
    compact.source_id = star.source_id;
    compact.ra = static_cast<float>(star.ra);
    compact.dec = static_cast<float>(star.dec);
    compact.parallax = static_cast<float>(star.parallax);
    compact.parallax_error = static_cast<float>(star.parallax_error);
    compact.pmra = static_cast<float>(star.pmra);
    compact.pmdec = static_cast<float>(star.pmdec);
    compact.phot_g_mean_mag = static_cast<float>(star.phot_g_mean_mag);
    compact.phot_bp_mean_mag = static_cast<float>(star.phot_bp_mean_mag);
    compact.phot_rp_mean_mag = static_cast<float>(star.phot_rp_mean_mag);
    return compact;
}

// =============================================================================
// Binary file format for efficient storage
// =============================================================================

// =============================================================================
// Logger class for detailed progress tracking
// =============================================================================

class CompilationLogger {
public:
    CompilationLogger() : verbose_(false), log_file_("") {}
    
    void setVerbose(bool verbose) { verbose_ = verbose; }
    
    void setLogFile(const std::string& path) {
        log_file_ = path;
        if (!path.empty()) {
            log_stream_.open(path, std::ios::app);
        }
    }
    
    void log(const std::string& level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] ";
        oss << "[" << level << "] " << message;
        
        std::string log_line = oss.str();
        
        if (verbose_) {
            std::cout << log_line << std::endl;
        }
        
        if (log_stream_.is_open()) {
            log_stream_ << log_line << std::endl;
            log_stream_.flush();
        }
    }
    
    void info(const std::string& message) { log("INFO", message); }
    void warn(const std::string& message) { log("WARN", message); }
    void error(const std::string& message) { log("ERROR", message); }
    void debug(const std::string& message) { log("DEBUG", message); }
    
private:
    bool verbose_;
    std::string log_file_;
    std::ofstream log_stream_;
};

// =============================================================================
// Compression utilities
// =============================================================================

namespace compression {

std::vector<uint8_t> compress(const void* data, size_t size, int level) {
    uLongf compressed_size = compressBound(size);
    std::vector<uint8_t> compressed(compressed_size);
    
    int result = compress2(
        compressed.data(), &compressed_size,
        reinterpret_cast<const Bytef*>(data), size,
        level
    );
    
    if (result != Z_OK) {
        throw GaiaException(ErrorCode::CACHE_ERROR, "Compression failed");
    }
    
    compressed.resize(compressed_size);
    return compressed;
}

std::vector<uint8_t> decompress(const void* data, size_t compressed_size, size_t original_size) {
    std::vector<uint8_t> decompressed(original_size);
    uLongf dest_size = original_size;
    
    int result = uncompress(
        decompressed.data(), &dest_size,
        reinterpret_cast<const Bytef*>(data), compressed_size
    );
    
    if (result != Z_OK) {
        throw GaiaException(ErrorCode::CACHE_ERROR, "Decompression failed");
    }
    
    return decompressed;
}

} // namespace compression

// =============================================================================
// Binary file format
// =============================================================================

namespace binary {

#pragma pack(push, 1)
struct FileHeader {
    char magic[8];           // "GAIACT3\0"
    uint32_t version;        // Format version
    uint32_t nside;          // HEALPix NSIDE
    uint32_t tile_id;        // HEALPix tile ID
    uint64_t num_stars;      // Stars in this tile
    uint64_t timestamp;      // Compilation timestamp
    uint8_t has_crossmatch;  // Cross-match data included (uint8_t instead of bool)
    uint8_t is_compressed;   // Data is compressed (uint8_t instead of bool)
    uint64_t compressed_size;// Compressed data size
    uint64_t original_size;  // Original data size
    char reserved[202];      // Reserved for future use (total 256 bytes)
    
    FileHeader() : version(1), nside(0), tile_id(0), num_stars(0), 
                   timestamp(0), has_crossmatch(false), is_compressed(false),
                   compressed_size(0), original_size(0) {
        std::memcpy(magic, "GAIACT3", 8);
        std::memset(reserved, 0, sizeof(reserved));
    }
};

struct StarRecord {
    int64_t source_id;
    float ra;
    float dec;
    float parallax;
    float parallax_error;
    float pmra;
    float pmdec;
    float phot_g;
    float phot_bp;
    float phot_rp;
    char sao[16];            // SAO number
    char tycho2[16];         // Tycho-2 ID
    char ucac4[16];          // UCAC4 ID
    char hipparcos[8];       // Hipparcos number
    char reserved[28];       // Reserved (total 128 bytes)
    
    StarRecord() : source_id(0), ra(0), dec(0), parallax(0), parallax_error(0),
                   pmra(0), pmdec(0), phot_g(0), phot_bp(0), phot_rp(0) {
        std::memset(sao, 0, sizeof(sao));
        std::memset(tycho2, 0, sizeof(tycho2));
        std::memset(ucac4, 0, sizeof(ucac4));
        std::memset(hipparcos, 0, sizeof(hipparcos));
        std::memset(reserved, 0, sizeof(reserved));
    }
    
    static StarRecord fromCompactStar(const CompactStar& star) {
        StarRecord rec;
        rec.source_id = star.source_id;
        rec.ra = star.ra;
        rec.dec = star.dec;
        rec.parallax = star.parallax;
        rec.parallax_error = star.parallax_error;
        rec.pmra = star.pmra;
        rec.pmdec = star.pmdec;
        rec.phot_g = star.phot_g_mean_mag;
        rec.phot_bp = star.phot_bp_mean_mag;
        rec.phot_rp = star.phot_rp_mean_mag;
        
        if (!star.cross_ids.sao.empty())
            std::strncpy(rec.sao, star.cross_ids.sao.c_str(), 15);
        if (!star.cross_ids.tycho2.empty())
            std::strncpy(rec.tycho2, star.cross_ids.tycho2.c_str(), 15);
        if (!star.cross_ids.ucac4.empty())
            std::strncpy(rec.ucac4, star.cross_ids.ucac4.c_str(), 15);
        if (!star.cross_ids.hipparcos.empty())
            std::strncpy(rec.hipparcos, star.cross_ids.hipparcos.c_str(), 7);
        
        return rec;
    }
    
    CompactStar toCompactStar() const {
        CompactStar star;
        star.source_id = source_id;
        star.ra = ra;
        star.dec = dec;
        star.parallax = parallax;
        star.parallax_error = parallax_error;
        star.pmra = pmra;
        star.pmdec = pmdec;
        star.phot_g_mean_mag = phot_g;
        star.phot_bp_mean_mag = phot_bp;
        star.phot_rp_mean_mag = phot_rp;
        
        if (sao[0]) star.cross_ids.sao = sao;
        if (tycho2[0]) star.cross_ids.tycho2 = tycho2;
        if (ucac4[0]) star.cross_ids.ucac4 = ucac4;
        if (hipparcos[0]) star.cross_ids.hipparcos = hipparcos;
        
        return star;
    }
};

static_assert(sizeof(FileHeader) == 256, "FileHeader must be 256 bytes");
static_assert(sizeof(StarRecord) == 128, "StarRecord must be 128 bytes");

#pragma pack(pop)

} // namespace binary

// =============================================================================
// Checkpoint System
// =============================================================================

struct Checkpoint {
    std::vector<int> completed_tiles;
    std::vector<int> pending_tiles;
    size_t total_stars;
    size_t stars_with_crossmatch;
    double max_magnitude;
    bool enable_crossmatch;
    uint64_t last_update;
    
    void save(const std::string& path) {
        std::ofstream ofs(path);
        ofs << "max_magnitude=" << max_magnitude << "\n";
        ofs << "enable_crossmatch=" << enable_crossmatch << "\n";
        ofs << "total_stars=" << total_stars << "\n";
        ofs << "stars_with_crossmatch=" << stars_with_crossmatch << "\n";
        ofs << "last_update=" << last_update << "\n";
        
        ofs << "completed_tiles=";
        for (size_t i = 0; i < completed_tiles.size(); ++i) {
            if (i > 0) ofs << ",";
            ofs << completed_tiles[i];
        }
        ofs << "\n";
    }
    
    bool load(const std::string& path) {
        if (!fs::exists(path)) return false;
        
        std::ifstream ifs(path);
        std::string line;
        
        while (std::getline(ifs, line)) {
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (key == "max_magnitude") max_magnitude = std::stod(value);
            else if (key == "enable_crossmatch") enable_crossmatch = (value == "1");
            else if (key == "total_stars") total_stars = std::stoull(value);
            else if (key == "stars_with_crossmatch") stars_with_crossmatch = std::stoull(value);
            else if (key == "last_update") last_update = std::stoull(value);
            else if (key == "completed_tiles") {
                std::istringstream ss(value);
                std::string id;
                while (std::getline(ss, id, ',')) {
                    completed_tiles.push_back(std::stoi(id));
                }
            }
        }
        
        return true;
    }
};

// =============================================================================
// GaiaCatalogCompiler::Impl
// =============================================================================

class GaiaCatalogCompiler::Impl {
public:
    std::string output_dir_;
    int nside_;
    GaiaRelease release_;
    GaiaClient* client_;
    double max_magnitude_;
    bool enable_crossmatch_;
    bool enable_compression_;
    int compression_level_;
    int parallel_workers_;
    
    std::atomic<bool> paused_;
    std::atomic<bool> cancelled_;
    
    Checkpoint checkpoint_;
    std::mutex checkpoint_mutex_;
    
    CompilationLogger logger_;
    
    // Progress tracking
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<size_t> bytes_downloaded_;
    std::atomic<size_t> bytes_compressed_;
    
    Impl(const std::string& output_dir, int nside, GaiaRelease release)
        : output_dir_(output_dir), nside_(nside), release_(release),
          client_(nullptr), max_magnitude_(99.0), enable_crossmatch_(true),
          enable_compression_(true), compression_level_(6),
          parallel_workers_(4), paused_(false), cancelled_(false),
          bytes_downloaded_(0), bytes_compressed_(0) {
        
        fs::create_directories(output_dir_);
        fs::create_directories(fs::path(output_dir_) / "tiles");
        fs::create_directories(fs::path(output_dir_) / "logs");
        
        // Load checkpoint if exists
        checkpoint_.load(getCheckpointPath());
        
        logger_.info("Initialized catalog compiler");
        logger_.info("Output directory: " + output_dir_);
        logger_.info("HEALPix NSIDE: " + std::to_string(nside_));
    }
    
    std::string getCheckpointPath() const {
        return output_dir_ + "/checkpoint.txt";
    }
    
    std::string getTilePath(int tile_id) const {
        std::ostringstream oss;
        oss << output_dir_ << "/tiles/tile_" 
            << std::setw(5) << std::setfill('0') << tile_id << ".bin";
        return oss.str();
    }
    
    void saveTile(int tile_id, const std::vector<CompactStar>& stars) {
        std::string path = getTilePath(tile_id);
        
        logger_.info("Saving tile " + std::to_string(tile_id) + 
                    " with " + std::to_string(stars.size()) + " stars");
        
        // Convert stars to binary records
        std::vector<binary::StarRecord> records;
        records.reserve(stars.size());
        for (const auto& star : stars) {
            records.push_back(binary::StarRecord::fromCompactStar(star));
        }
        
        size_t data_size = records.size() * sizeof(binary::StarRecord);
        bytes_downloaded_ += data_size;
        
        binary::FileHeader header;
        header.nside = nside_;
        header.tile_id = tile_id;
        header.num_stars = stars.size();
        header.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        header.has_crossmatch = enable_crossmatch_;
        header.is_compressed = enable_compression_;
        header.original_size = data_size;
        
        std::ofstream ofs(path, std::ios::binary);
        
        if (enable_compression_ && !records.empty()) {
            logger_.debug("Compressing tile " + std::to_string(tile_id) + 
                         " (level " + std::to_string(compression_level_) + ")");
            
            auto compressed = compression::compress(
                records.data(), data_size, compression_level_
            );
            
            header.compressed_size = compressed.size();
            bytes_compressed_ += compressed.size();
            
            double ratio = 100.0 * (1.0 - double(compressed.size()) / data_size);
            logger_.info("Compressed tile " + std::to_string(tile_id) + 
                        ": " + std::to_string(data_size / 1024) + " KB -> " +
                        std::to_string(compressed.size() / 1024) + " KB (" +
                        std::to_string(static_cast<int>(ratio)) + "% saved)");
            
            ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
            ofs.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        } else {
            header.compressed_size = data_size;
            bytes_compressed_ += data_size;
            
            ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
            ofs.write(reinterpret_cast<const char*>(records.data()), data_size);
        }
        
        logger_.debug("Saved tile " + std::to_string(tile_id) + " to disk");
    }
    
    EquatorialCoordinates getTileCenter(int tile_id) {
        // Convert HEALPix tile ID to center coordinates
        // theta: colatitude [0, pi], phi: longitude [0, 2*pi]
        int nside = nside_;
        int npix = 12 * nside * nside;
        
        if (tile_id < 0 || tile_id >= npix) {
            throw GaiaException(ErrorCode::INVALID_PARAMS,
                              "Invalid tile ID: " + std::to_string(tile_id));
        }
        
        // Simplified HEALPix pix2ang conversion (ring scheme)
        // For accurate implementation, use HEALPix library
        // This is a placeholder approximation
        
        double theta, phi;
        int ncap = 2 * nside * (nside - 1);
        int npix_ring = 4 * nside;
        
        if (tile_id < ncap) {
            // North polar cap
            int iring = (1 + (int)std::sqrt(1 + 2 * tile_id)) / 2;
            int iphi = tile_id - 2 * iring * (iring - 1);
            theta = std::acos(1.0 - (iring * iring) / (3.0 * nside * nside));
            phi = (iphi + 0.5) * M_PI / (2.0 * iring);
        } else if (tile_id < npix - ncap) {
            // Equatorial belt
            int ip = tile_id - ncap;
            int iring = ip / npix_ring + nside;
            int iphi = ip % npix_ring;
            double z = (2 * nside - iring) / (1.5 * nside);
            theta = std::acos(z);
            phi = (iphi + 0.5) * M_PI / (2.0 * nside);
        } else {
            // South polar cap
            int ip = npix - tile_id;
            int iring = (1 + (int)std::sqrt(2 * ip - 1)) / 2;
            int iphi = 4 * iring - (npix - tile_id - 2 * iring * (iring - 1));
            theta = std::acos(-1.0 + (iring * iring) / (3.0 * nside * nside));
            phi = (iphi - 0.5) * M_PI / (2.0 * iring);
        }
        
        EquatorialCoordinates coord;
        coord.ra = phi * 180.0 / M_PI;
        coord.dec = 90.0 - theta * 180.0 / M_PI;
        
        // Normalize RA to [0, 360)
        while (coord.ra < 0) coord.ra += 360.0;
        while (coord.ra >= 360.0) coord.ra -= 360.0;
        
        return coord;
    }
    
    std::vector<CompactStar> downloadTile(int tile_id) {
        if (!client_) {
            throw GaiaException(ErrorCode::INVALID_PARAMS,
                              "Client not set");
        }
        
        auto center = getTileCenter(tile_id);
        
        // Calculate tile radius in degrees
        // For NSIDE=32, each tile covers approximately 3.4 degrees
        double tile_radius = 180.0 / M_PI * std::sqrt(M_PI / (3.0 * nside_ * nside_));
        
        // Build ADQL query for this tile region
        // Query cone around tile center with appropriate radius
        // NOTE: Cross-match is currently disabled due to complex table structure
        // in Gaia Archive. Will be implemented in a future version with proper
        // cross-match table queries.
        std::ostringstream adql;
        adql << "SELECT source_id, ra, dec, parallax, parallax_error, "
             << "pmra, pmdec, phot_g_mean_mag, phot_bp_mean_mag, phot_rp_mean_mag "
             << "FROM gaiadr3.gaia_source "
             << "WHERE CONTAINS(POINT('ICRS', ra, dec), "
             << "CIRCLE('ICRS', " << center.ra << ", " << center.dec << ", " 
             << tile_radius << ")) = 1 ";
        
        if (max_magnitude_ < 99.0) {
            adql << "AND phot_g_mean_mag < " << max_magnitude_ << " ";
        }
        
        adql << "ORDER BY source_id";
        
        // Execute query
        auto gaia_stars = client_->queryADQL(adql.str());
        
        // Convert to CompactStar format
        std::vector<CompactStar> compact_stars;
        compact_stars.reserve(gaia_stars.size());
        
        for (const auto& star : gaia_stars) {
            CompactStar cs;
            cs.source_id = star.source_id;
            cs.ra = star.ra;
            cs.dec = star.dec;
            cs.parallax = star.parallax;
            cs.parallax_error = star.parallax_error;
            cs.pmra = star.pmra;
            cs.pmdec = star.pmdec;
            cs.phot_g_mean_mag = star.phot_g_mean_mag;
            cs.phot_bp_mean_mag = star.phot_bp_mean_mag;
            cs.phot_rp_mean_mag = star.phot_rp_mean_mag;
            
            // Cross-match data would be extracted here from query results
            // For now, using empty cross-match
            cs.cross_ids = CrossMatchIds();
            
            compact_stars.push_back(cs);
        }
        
        bytes_downloaded_ += gaia_stars.size() * sizeof(binary::StarRecord);
        
        return compact_stars;
    }
    
    std::vector<CompactStar> loadTile(int tile_id) {
        std::string path = getTilePath(tile_id);
        std::ifstream ifs(path, std::ios::binary);
        
        if (!ifs) {
            logger_.error("Cannot open tile " + std::to_string(tile_id));
            return {};
        }
        
        binary::FileHeader header;
        ifs.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        std::vector<binary::StarRecord> records(header.num_stars);
        
        if (header.is_compressed) {
            logger_.debug("Decompressing tile " + std::to_string(tile_id));
            
            std::vector<uint8_t> compressed(header.compressed_size);
            ifs.read(reinterpret_cast<char*>(compressed.data()), header.compressed_size);
            
            auto decompressed = compression::decompress(
                compressed.data(), header.compressed_size, header.original_size
            );
            
            std::memcpy(records.data(), decompressed.data(), header.original_size);
        } else {
            size_t data_size = header.num_stars * sizeof(binary::StarRecord);
            ifs.read(reinterpret_cast<char*>(records.data()), data_size);
        }
        
        std::vector<CompactStar> stars;
        stars.reserve(header.num_stars);
        for (const auto& rec : records) {
            stars.push_back(rec.toCompactStar());
        }
        
        logger_.debug("Loaded " + std::to_string(stars.size()) + 
                     " stars from tile " + std::to_string(tile_id));
        
        return stars;
    }
    
    void downloadCrossMatch(CompactStar& star) {
        if (!enable_crossmatch_ || !client_) return;
        
        // Query Gaia DR3 cross-match tables
        // Note: This requires additional ADQL queries to gaiadr3.dr2_neighbourhood, etc.
        // Simplified implementation - in production, batch these queries
        
        // For now, just a placeholder showing the structure
        // Real implementation would query:
        // - gaiadr3.sao_neighbourhood
        // - gaiadr3.tycho2_neighbourhood  
        // - gaiadr3.ucac4_neighbourhood
        // - gaiadr3.hipparcos2_neighbourhood
    }
};

// =============================================================================
// GaiaCatalogCompiler Public API
// =============================================================================

GaiaCatalogCompiler::GaiaCatalogCompiler(
    const std::string& output_dir,
    int nside,
    GaiaRelease release)
    : pImpl_(std::make_unique<Impl>(output_dir, nside, release)) {}

GaiaCatalogCompiler::~GaiaCatalogCompiler() = default;

GaiaCatalogCompiler::GaiaCatalogCompiler(GaiaCatalogCompiler&&) noexcept = default;
GaiaCatalogCompiler& GaiaCatalogCompiler::operator=(GaiaCatalogCompiler&&) noexcept = default;

void GaiaCatalogCompiler::setClient(GaiaClient* client) {
    pImpl_->client_ = client;
}

void GaiaCatalogCompiler::setMaxMagnitude(double max_mag) {
    pImpl_->max_magnitude_ = max_mag;
}

void GaiaCatalogCompiler::setEnableCrossMatch(bool enable) {
    pImpl_->enable_crossmatch_ = enable;
    pImpl_->logger_.info(std::string("Cross-match ") + (enable ? "enabled" : "disabled"));
}

void GaiaCatalogCompiler::setEnableCompression(bool enable) {
    pImpl_->enable_compression_ = enable;
    pImpl_->logger_.info(std::string("Compression ") + (enable ? "enabled" : "disabled"));
}

void GaiaCatalogCompiler::setCompressionLevel(int level) {
    pImpl_->compression_level_ = std::max(0, std::min(level, 9));
    pImpl_->logger_.info("Compression level set to " + std::to_string(pImpl_->compression_level_));
}

void GaiaCatalogCompiler::setLogging(const std::string& log_file, bool verbose) {
    pImpl_->logger_.setVerbose(verbose);
    if (!log_file.empty()) {
        pImpl_->logger_.setLogFile(log_file);
        pImpl_->logger_.info("Logging to file: " + log_file);
    }
}

void GaiaCatalogCompiler::setParallelWorkers(int workers) {
    pImpl_->parallel_workers_ = std::max(1, std::min(workers, 16));
    pImpl_->logger_.info("Parallel workers set to " + std::to_string(pImpl_->parallel_workers_));
}

CompilationStats GaiaCatalogCompiler::compile(CompilationCallback callback) {
    if (!pImpl_->client_) {
        throw GaiaException(ErrorCode::INVALID_PARAMS, 
                          "Client not set. Call setClient() first.");
    }
    
    pImpl_->logger_.info("=== Starting compilation ===");
    pImpl_->logger_.info("Max magnitude: " + std::to_string(pImpl_->max_magnitude_));
    pImpl_->logger_.info("Cross-match: " + std::string(pImpl_->enable_crossmatch_ ? "yes" : "no"));
    pImpl_->logger_.info("Compression: " + std::string(pImpl_->enable_compression_ ? "yes" : "no"));
    
    pImpl_->cancelled_ = false;
    pImpl_->paused_ = false;
    pImpl_->start_time_ = std::chrono::steady_clock::now();
    pImpl_->bytes_downloaded_ = 0;
    pImpl_->bytes_compressed_ = 0;
    
    // Generate list of all tiles
    int total_tiles = 12 * pImpl_->nside_ * pImpl_->nside_;
    std::vector<int> all_tiles(total_tiles);
    for (int i = 0; i < total_tiles; ++i) all_tiles[i] = i;
    
    // Remove already completed tiles
    std::unordered_set<int> completed_set(
        pImpl_->checkpoint_.completed_tiles.begin(),
        pImpl_->checkpoint_.completed_tiles.end()
    );
    
    std::vector<int> pending;
    for (int tile : all_tiles) {
        if (completed_set.find(tile) == completed_set.end()) {
            pending.push_back(tile);
        }
    }
    
    pImpl_->logger_.info("Total tiles: " + std::to_string(total_tiles));
    pImpl_->logger_.info("Completed tiles: " + std::to_string(completed_set.size()));
    pImpl_->logger_.info("Pending tiles: " + std::to_string(pending.size()));
    
    if (callback) {
        CompilationProgress progress;
        progress.percent = 0;
        progress.phase = "Initialization";
        progress.stars_processed = pImpl_->checkpoint_.total_stars;
        progress.current_tile_id = 0;
        progress.total_tiles = total_tiles;
        progress.completed_tiles = completed_set.size();
        progress.current_ra = 0;
        progress.current_dec = 0;
        progress.download_speed_mbps = 0;
        progress.eta_seconds = -1;
        progress.memory_used_mb = 0;
        progress.message = "Found " + std::to_string(pending.size()) + " tiles to process";
        callback(progress);
    }
    
    // Process tiles
    for (size_t i = 0; i < pending.size() && !pImpl_->cancelled_; ++i) {
        while (pImpl_->paused_ && !pImpl_->cancelled_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (pImpl_->cancelled_) break;
        
        int tile_id = pending[i];
        auto center = pImpl_->getTileCenter(tile_id);
        
        try {
            pImpl_->logger_.info("Processing tile " + std::to_string(tile_id) + 
                               " (" + std::to_string(i+1) + "/" + std::to_string(pending.size()) + 
                               ") at RA=" + std::to_string(center.ra) + 
                               ", Dec=" + std::to_string(center.dec));
            
            auto tile_start = std::chrono::steady_clock::now();
            
            // Download tile data
            auto stars = pImpl_->downloadTile(tile_id);
            
            // Save tile with optional compression
            pImpl_->saveTile(tile_id, stars);
            
            auto tile_end = std::chrono::steady_clock::now();
            auto tile_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                tile_end - tile_start).count();
            
            // Update statistics
            {
                std::lock_guard<std::mutex> lock(pImpl_->checkpoint_mutex_);
                pImpl_->checkpoint_.completed_tiles.push_back(tile_id);
                pImpl_->checkpoint_.total_stars += stars.size();
                pImpl_->checkpoint_.save(pImpl_->getCheckpointPath());
            }
            
            // Calculate progress metrics
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - pImpl_->start_time_).count();
            
            double download_speed_mbps = 0;
            if (elapsed > 0 && pImpl_->bytes_downloaded_ > 0) {
                download_speed_mbps = (pImpl_->bytes_downloaded_ / (1024.0 * 1024.0)) / elapsed;
            }
            
            double eta_seconds = -1;
            if (i > 0) {
                double avg_time_per_tile = (double)elapsed / (i + 1);
                eta_seconds = avg_time_per_tile * (pending.size() - i - 1);
            }
            
            pImpl_->logger_.info("Tile " + std::to_string(tile_id) + " completed: " + 
                               std::to_string(stars.size()) + " stars in " + 
                               std::to_string(tile_time) + " ms");
            
            if (callback) {
                CompilationProgress progress;
                progress.percent = (100 * (i + 1)) / pending.size();
                progress.phase = "Processing";
                progress.stars_processed = pImpl_->checkpoint_.total_stars;
                progress.current_tile_id = tile_id;
                progress.total_tiles = 12 * pImpl_->nside_ * pImpl_->nside_;
                progress.completed_tiles = completed_set.size() + i + 1;
                progress.current_ra = center.ra;
                progress.current_dec = center.dec;
                progress.download_speed_mbps = download_speed_mbps;
                progress.eta_seconds = eta_seconds;
                progress.memory_used_mb = 0;  // TODO: implement memory tracking
                progress.message = "Tile " + std::to_string(tile_id) + ": " + 
                                 std::to_string(stars.size()) + " stars (" + 
                                 std::to_string(tile_time) + " ms)";
                callback(progress);
            }
            
        } catch (const std::exception& e) {
            pImpl_->logger_.error("Error on tile " + std::to_string(tile_id) + ": " + 
                                std::string(e.what()));
            if (callback) {
                CompilationProgress progress;
                progress.percent = -1;
                progress.phase = "Error";
                progress.stars_processed = pImpl_->checkpoint_.total_stars;
                progress.current_tile_id = tile_id;
                progress.total_tiles = 12 * pImpl_->nside_ * pImpl_->nside_;
                progress.completed_tiles = completed_set.size() + i;
                progress.current_ra = center.ra;
                progress.current_dec = center.dec;
                progress.download_speed_mbps = 0;
                progress.eta_seconds = -1;
                progress.memory_used_mb = 0;
                progress.message = std::string(e.what());
                callback(progress);
            }
        }
    }
    
    // Finalization
    auto end_time = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - pImpl_->start_time_).count();
    
    pImpl_->logger_.info("=== Compilation finished ===");
    pImpl_->logger_.info("Status: " + std::string(pImpl_->cancelled_ ? "Cancelled" : "Completed"));
    pImpl_->logger_.info("Total stars: " + std::to_string(pImpl_->checkpoint_.total_stars));
    pImpl_->logger_.info("Total time: " + std::to_string(total_time) + " seconds");
    
    if (pImpl_->enable_compression_ && pImpl_->bytes_compressed_ > 0) {
        double compression_ratio = (double)pImpl_->bytes_downloaded_ / pImpl_->bytes_compressed_;
        size_t saved_mb = (pImpl_->bytes_downloaded_ - pImpl_->bytes_compressed_) / (1024 * 1024);
        pImpl_->logger_.info("Compression: " + std::to_string(compression_ratio) + ":1 ratio, " +
                           std::to_string(saved_mb) + " MB saved");
    }
    
    if (callback && !pImpl_->cancelled_) {
        CompilationProgress progress;
        progress.percent = 100;
        progress.phase = "Complete";
        progress.stars_processed = pImpl_->checkpoint_.total_stars;
        progress.current_tile_id = 0;
        progress.total_tiles = 12 * pImpl_->nside_ * pImpl_->nside_;
        progress.completed_tiles = pImpl_->checkpoint_.completed_tiles.size();
        progress.current_ra = 0;
        progress.current_dec = 0;
        progress.download_speed_mbps = 0;
        progress.eta_seconds = 0;
        progress.memory_used_mb = 0;
        progress.message = "Catalog compilation complete: " + 
                         std::to_string(pImpl_->checkpoint_.total_stars) + " stars";
        callback(progress);
    }
    
    auto stats = getStatistics();
    stats.compilation_time_seconds = total_time;
    return stats;
}

bool GaiaCatalogCompiler::isComplete() const {
    int total_tiles = 12 * pImpl_->nside_ * pImpl_->nside_;
    return pImpl_->checkpoint_.completed_tiles.size() >= static_cast<size_t>(total_tiles);
}

double GaiaCatalogCompiler::getProgress() const {
    int total_tiles = 12 * pImpl_->nside_ * pImpl_->nside_;
    return (100.0 * pImpl_->checkpoint_.completed_tiles.size()) / total_tiles;
}

CompilationStats GaiaCatalogCompiler::getStatistics() const {
    CompilationStats stats;
    stats.total_stars = pImpl_->checkpoint_.total_stars;
    stats.stars_with_crossmatch = pImpl_->checkpoint_.stars_with_crossmatch;
    stats.healpix_tiles = pImpl_->checkpoint_.completed_tiles.size();
    stats.coverage_percent = getProgress();
    
    // Calculate disk size
    for (int tile_id : pImpl_->checkpoint_.completed_tiles) {
        std::string path = pImpl_->getTilePath(tile_id);
        if (fs::exists(path)) {
            stats.disk_size_bytes += fs::file_size(path);
        }
    }
    
    return stats;
}

void GaiaCatalogCompiler::pause() {
    pImpl_->paused_ = true;
}

void GaiaCatalogCompiler::resume() {
    pImpl_->paused_ = false;
}

void GaiaCatalogCompiler::cancel() {
    pImpl_->cancelled_ = true;
}

std::vector<int> GaiaCatalogCompiler::getCompletedTiles() const {
    return pImpl_->checkpoint_.completed_tiles;
}

std::vector<int> GaiaCatalogCompiler::getPendingTiles() const {
    int total_tiles = 12 * pImpl_->nside_ * pImpl_->nside_;
    std::unordered_set<int> completed_set(
        pImpl_->checkpoint_.completed_tiles.begin(),
        pImpl_->checkpoint_.completed_tiles.end()
    );
    
    std::vector<int> pending;
    for (int i = 0; i < total_tiles; ++i) {
        if (completed_set.find(i) == completed_set.end()) {
            pending.push_back(i);
        }
    }
    
    return pending;
}

size_t GaiaCatalogCompiler::estimateDiskSpace(double max_magnitude) {
    // Rough estimate: ~1.8 billion stars in Gaia DR3
    // At mag 18: ~hundreds of millions
    // At mag 12: ~millions
    // Each star: 128 bytes in binary format
    
    double star_fraction;
    if (max_magnitude >= 20) star_fraction = 1.0;
    else if (max_magnitude >= 18) star_fraction = 0.5;
    else if (max_magnitude >= 15) star_fraction = 0.1;
    else if (max_magnitude >= 12) star_fraction = 0.01;
    else star_fraction = 0.001;
    
    size_t estimated_stars = static_cast<size_t>(1.8e9 * star_fraction);
    return estimated_stars * sizeof(binary::StarRecord);
}

} // namespace gaia
} // namespace ioc
