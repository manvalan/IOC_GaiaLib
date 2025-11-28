/**
 * @file debug_magnitude.cpp
 * @brief Debug: verifica perché le magnitudini sono 0
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include "../include/ioc_gaialib/gaia_mag18_catalog_v2.h"

using namespace ioc::gaia;

int main() {
    std::string home = std::getenv("HOME");
    std::string multifile_dir = home + "/.catalog/gaia_mag18_v2_multifile";
    std::string compressed_file = home + "/.catalog/gaia_mag18_v2.mag18v2";
    
    std::cout << "=== DEBUG MAGNITUDINI ===" << std::endl;
    
    // 1. Leggi direttamente un chunk per vedere i dati raw
    std::cout << "\n--- 1. Lettura diretta chunk_000.dat ---" << std::endl;
    std::string chunk_path = multifile_dir + "/chunks/chunk_000.dat";
    std::ifstream chunk_file(chunk_path, std::ios::binary);
    
    if (!chunk_file) {
        std::cerr << "ERRORE: impossibile aprire " << chunk_path << std::endl;
        return 1;
    }
    
    // Leggi i primi 5 record
    std::vector<Mag18RecordV2> records(5);
    chunk_file.read(reinterpret_cast<char*>(records.data()), 5 * sizeof(Mag18RecordV2));
    
    std::cout << "sizeof(Mag18RecordV2) = " << sizeof(Mag18RecordV2) << " bytes\n\n";
    
    for (int i = 0; i < 5; i++) {
        const auto& r = records[i];
        std::cout << "Record " << i << ":\n";
        std::cout << "  source_id: " << r.source_id << "\n";
        std::cout << "  ra: " << std::fixed << std::setprecision(6) << r.ra << "\n";
        std::cout << "  dec: " << r.dec << "\n";
        std::cout << "  g_mag: " << r.g_mag << "\n";
        std::cout << "  bp_mag: " << r.bp_mag << "\n";
        std::cout << "  rp_mag: " << r.rp_mag << "\n";
        std::cout << "  parallax: " << r.parallax << "\n";
        std::cout << "  pmra: " << r.pmra << "\n";
        std::cout << "  pmdec: " << r.pmdec << "\n";
        std::cout << "  healpix_pixel: " << r.healpix_pixel << "\n";
        std::cout << std::endl;
    }
    
    // 2. Leggi i byte raw per confronto
    std::cout << "\n--- 2. Byte raw del primo record ---" << std::endl;
    chunk_file.seekg(0);
    unsigned char raw_bytes[80];
    chunk_file.read(reinterpret_cast<char*>(raw_bytes), 80);
    
    for (int i = 0; i < 80; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)raw_bytes[i] << " ";
        if ((i + 1) % 16 == 0) std::cout << "\n";
    }
    std::cout << std::dec << std::endl;
    
    // 3. Interpreta manualmente
    std::cout << "\n--- 3. Interpretazione manuale ---" << std::endl;
    uint64_t* source_id_ptr = reinterpret_cast<uint64_t*>(&raw_bytes[0]);
    double* ra_ptr = reinterpret_cast<double*>(&raw_bytes[8]);
    double* dec_ptr = reinterpret_cast<double*>(&raw_bytes[16]);
    float* g_mag_ptr = reinterpret_cast<float*>(&raw_bytes[24]);
    float* bp_mag_ptr = reinterpret_cast<float*>(&raw_bytes[28]);
    float* rp_mag_ptr = reinterpret_cast<float*>(&raw_bytes[32]);
    
    std::cout << "source_id (offset 0): " << *source_id_ptr << "\n";
    std::cout << "ra (offset 8): " << *ra_ptr << "\n";
    std::cout << "dec (offset 16): " << *dec_ptr << "\n";
    std::cout << "g_mag (offset 24): " << *g_mag_ptr << "\n";
    std::cout << "bp_mag (offset 28): " << *bp_mag_ptr << "\n";
    std::cout << "rp_mag (offset 32): " << *rp_mag_ptr << "\n";
    
    // 4. Prova il catalogo compresso per confronto
    std::cout << "\n--- 4. Confronto con catalogo compresso ---" << std::endl;
    std::ifstream compressed_f(compressed_file, std::ios::binary);
    if (compressed_f) {
        // Leggi header
        Mag18CatalogHeaderV2 header;
        compressed_f.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        std::cout << "Header magic: " << std::string(header.magic, 8) << "\n";
        std::cout << "Total stars: " << header.total_stars << "\n";
        std::cout << "Stars per chunk: " << header.stars_per_chunk << "\n";
        std::cout << "Total chunks: " << header.total_chunks << "\n";
    } else {
        std::cout << "Catalogo compresso non trovato: " << compressed_file << "\n";
    }
    
    return 0;
}
