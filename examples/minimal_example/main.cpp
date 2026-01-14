#include <ioc_gaialib/unified_gaia_catalog.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <string>

// Namespace alias for cleaner code
using namespace ioc::gaia;

int main(int argc, char* argv[]) {
    // 1. Definisci il path al nuovo catalogo SQLite
    std::string home = std::getenv("HOME");
    std::string db_path = home + "/.catalog/crossreference/gaia_dr3_occult_pro.db";
    
    // Se l'utente passa un argomento, usalo come override (opzionale)
    if (argc > 1) {
        db_path = argv[1];
    }
    
    std::cout << "Using SQLite database: " << db_path << std::endl;

    // 2. Prepara la configurazione JSON per SQLite
    // Il formato "sqlite_dr3" è quello raccomandato: veloce, compatto e con cross-references
    std::string config = R"({
        "catalog_type": "sqlite_dr3",
        "sqlite_file_path": ")" + db_path + R"(",
        "log_level": "info"
    })";

    // 3. Inizializza il catalogo (Singleton)
    // Va fatto UNA VOLTA SOLA all'inizio del programma
    std::cout << "Initializing UnifiedGaiaCatalog..." << std::endl;
    if (!UnifiedGaiaCatalog::initialize(config)) {
        std::cerr << "Errore critico: Impossibile inizializzare il catalogo!" << std::endl;
        std::cerr << "Assicurati che la directory esista: " << catalog_dir << std::endl;
        return 1;
    }

    // 4. Ottieni l'istanza del catalogo
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    try {
        // --- ESEMPIO 1: Cone Search (Ricerca per cono) ---
        // Cerchiamo stelle intorno a M42 (Nebulosa di Orione)
        // RA: 83.82, Dec: -5.39, Raggio: 1.0 grado
        std::cout << "\n--- Esegui Cone Search ---" << std::endl;
        
        QueryParams params;
        params.ra_center = 83.8220;
        params.dec_center = -5.3911;
        params.radius = 1.0;
        params.max_magnitude = 15.0; // Filtra stelle troppo deboli
        
        auto stars = catalog.queryCone(params);
        std::cout << "Trovate " << stars.size() << " stelle nel cono." << std::endl;

        // --- ESEMPIO 2: Ricerca per Source ID ---
        // Cerchiamo una stella specifica se conosciamo il suo ID Gaia
        std::cout << "\n--- Ricerca per Source ID ---" << std::endl;
        uint64_t target_id = 2947050466531873024ULL; // Betelgeuse (circa) o altra stella nota
        
        auto star_opt = catalog.queryBySourceId(target_id);
        if (star_opt) {
            const auto& star = *star_opt;
            std::cout << "Stella trovata!" << std::endl;
            std::cout << "ID:  " << star.source_id << std::endl;
            std::cout << "RA:  " << std::fixed << std::setprecision(6) << star.ra << " deg" << std::endl;
            std::cout << "Dec: " << star.dec << " deg" << std::endl;
            std::cout << "G:   " << std::setprecision(3) << star.phot_g_mean_mag << " mag" << std::endl;
        } else {
            std::cout << "Stella con ID " << target_id << " non trovata (potrebbe essere fuori magnitudine limite)." << std::endl;
        }

        // --- ESEMPIO 3: Ricerca per Nome (opzionale, se supportato) ---
        std::cout << "\n--- Ricerca per Nome ---" << std::endl;
        auto sirius = catalog.queryByName("Sirius");
        if (sirius) {
             std::cout << "Sirio trovata: RA=" << sirius->ra << ", Dec=" << sirius->dec << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Errore durante le query: " << e.what() << std::endl;
    }

    // 5. Cleanup finale (opzionale, il sistema operativo pulisce comunque, ma buona norma)
    UnifiedGaiaCatalog::shutdown();
    std::cout << "\nProgramma terminato." << std::endl;

    return 0;
}
