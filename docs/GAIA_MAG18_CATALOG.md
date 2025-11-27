# Gaia Magnitude 18 Compressed Catalog

## Overview

Sistema per creare e utilizzare un sottoinsieme compresso del catalogo Gaia EDR3 contenente **solo stelle fino alla magnitudine 18**.

### Vantaggi
- ✅ **Compressione 50-70%**: File molto più piccolo rispetto al catalogo completo
- ✅ **Ricerca veloce per source_id**: Binary search O(log N)
- ✅ **Formato ottimizzato**: 52 bytes per stella invece di ~180 bytes del formato originale
- ✅ **Accesso rapido**: Decompressione on-the-fly con zlib
- ✅ **Portabile**: Un singolo file .gz

### Limitazioni
- ⚠️ **Cone search lente**: Scansione lineare (adatto per raggio < 5°)
- ⚠️ **Parametri limitati**: Solo i più essenziali (RA, Dec, mags, parallax)
- ⚠️ **Magnitudine fissa**: Limite a G ≤ 18.0 definito alla creazione

---

## Creazione del Catalogo

### Prerequisiti
- GRAPPA3E catalogo completo estratto in `~/catalogs/GRAPPA3E`
- Spazio disco: ~2-3 GB per il catalogo mag ≤ 18
- Tempo di creazione: ~30-60 minuti

### Comando

```bash
cd /Users/michelebigi/VisualStudio\ Code/GitHub/IOC_GaiaLib/build/examples
./build_mag18_catalog ~/catalogs/GRAPPA3E gaia_mag18.cat.gz
```

### Output Atteso

```
╔════════════════════════════════════════════════════════════╗
║  GRAPPA3E → Gaia Mag18 Compressed Catalog Builder         ║
╚════════════════════════════════════════════════════════════╝

📂 Phase 1: Loading GRAPPA3E catalog...
✅ Catalog loaded: /Users/michelebigi/catalogs/GRAPPA3E

🔍 Phase 2: Scanning tiles for mag ≤ 18 stars...
  Scanning [==================================================] 100% (61202/61202)
✅ Scanning complete: 150000000 stars with mag ≤ 18.0

🔀 Phase 3: Sorting by source_id...
✅ Sorted 150000000 stars

📦 Phase 4: Converting to binary format...
  Converting [==================================================] 100% (150000000/150000000)
✅ Converted 150000000 stars to binary format

🗜️  Phase 5: Compressing data...
  Compressing [==================================================] 100% (7800000000/7800000000)
✅ Compression complete

╔════════════════════════════════════════════════════════════╗
║  CATALOG CREATION COMPLETE                                 ║
╚════════════════════════════════════════════════════════════╝

📊 Statistics:
  Stars included:     150000000
  Magnitude limit:    ≤ 18.0
  Uncompressed size:  7800 MB
  Compressed size:    2340 MB
  Compression ratio:  70.0%
  Output file:        gaia_mag18.cat.gz

✅ Ready to use with GaiaMag18Catalog class!
```

---

## Utilizzo del Catalogo

### Esempio Base

```cpp
#include "ioc_gaialib/gaia_mag18_catalog.h"
#include <iostream>

using namespace ioc_gaialib;
using namespace ioc::gaia;

int main() {
    // 1. Carica il catalogo compresso
    GaiaMag18Catalog catalog("gaia_mag18.cat.gz");
    
    if (!catalog.isLoaded()) {
        std::cerr << "Errore: impossibile caricare il catalogo\n";
        return 1;
    }
    
    // 2. Ottieni statistiche
    auto stats = catalog.getStatistics();
    std::cout << "Stelle totali: " << stats.total_stars << "\n";
    std::cout << "Limite magnitudine: " << stats.mag_limit << "\n";
    std::cout << "Compressione: " << stats.compression_ratio << "%\n";
    
    // 3. Cerca per source_id (VELOCE - binary search)
    auto star = catalog.queryBySourceId(12345678901234);
    if (star) {
        std::cout << "Trovata: RA=" << star->ra 
                  << " Dec=" << star->dec 
                  << " G=" << star->phot_g_mean_mag << "\n";
    }
    
    // 4. Cone search (più lento - scansione lineare)
    auto stars = catalog.queryCone(180.0, 0.0, 1.0, 100);
    std::cout << "Trovate " << stars.size() << " stelle in 1° di raggio\n";
    
    // 5. Cerca stelle brillanti
    auto bright = catalog.queryConeWithMagnitude(
        180.0, 0.0, 2.0,  // RA, Dec, raggio
        10.0, 12.0,       // mag min, mag max
        10                // max risultati
    );
    
    // 6. Trova le N più brillanti
    auto brightest = catalog.queryBrightest(180.0, 0.0, 2.0, 5);
    for (const auto& s : brightest) {
        std::cout << "G=" << s.phot_g_mean_mag << "\n";
    }
    
    // 7. Conta stelle in una regione
    size_t count = catalog.countInCone(180.0, 0.0, 5.0);
    std::cout << "Stelle in 5° di raggio: " << count << "\n";
    
    return 0;
}
```

### Test del Catalogo

```bash
cd /Users/michelebigi/VisualStudio\ Code/GitHub/IOC_GaiaLib/build/examples
./test_mag18_catalog gaia_mag18.cat.gz
```

---

## Formato File

### Header (64 bytes)

```cpp
struct Mag18CatalogHeader {
    char magic[8];           // "GAIA18\0\0"
    uint32_t version;        // 1
    uint64_t total_stars;    // Numero totale stelle
    double mag_limit;        // 18.0
    uint64_t data_offset;    // 64 (dopo header)
    uint64_t data_size;      // Dimensione dati non compressi
    uint8_t reserved[20];    // Riservato
};
```

### Record Stella (52 bytes)

```cpp
struct Mag18Record {
    uint64_t source_id;      // 8 bytes - Gaia DR3 source_id
    double ra;               // 8 bytes - Right Ascension (°)
    double dec;              // 8 bytes - Declination (°)
    double g_mag;            // 8 bytes - G magnitude
    double bp_mag;           // 8 bytes - BP magnitude
    double rp_mag;           // 8 bytes - RP magnitude
    float parallax;          // 4 bytes - Parallax (mas)
};
```

### Organizzazione Dati

```
[Header: 64 bytes]
[Compressed Records]
  ↓ gzip decompression
[Record 1: 52 bytes] ← source_id crescente
[Record 2: 52 bytes]
[Record 3: 52 bytes]
...
[Record N: 52 bytes]
```

Le stelle sono **ordinate per source_id crescente**, permettendo binary search rapida.

---

## Performance

### Query per source_id
- **Complessità**: O(log N)
- **Tempo**: <1 ms anche per cataloghi con 100M+ stelle
- **Metodo**: Binary search su catalogo ordinato

### Cone Search
- **Complessità**: O(N) - scansione completa
- **Tempo**: ~10-30 secondi per 100M stelle (dipende da raggio)
- **Raccomandazione**: Usare solo per raggi < 5° o considerare indicizzazione spaziale

### Count in Cone
- **Complessità**: O(N)
- **Tempo**: Simile a cone search ma più veloce (no allocazione)

### Query con Filtro Magnitudine
- **Complessità**: O(N)
- **Ottimizzazione**: Filtro magnitudine applicato prima del calcolo distanza

### Brightest Stars
- **Complessità**: O(N log K) dove K = numero stelle richieste
- **Metodo**: Partial sort dopo cone search

---

## API Reference

### Costruttore

```cpp
GaiaMag18Catalog(const std::string& catalog_file);
```
Carica il catalogo compresso. Il file viene aperto in lettura.

### isLoaded()

```cpp
bool isLoaded() const;
```
Verifica se il catalogo è stato caricato correttamente.

### getStatistics()

```cpp
Statistics getStatistics() const;
```
Ritorna statistiche sul catalogo:
- `total_stars`: numero stelle
- `mag_limit`: limite magnitudine
- `file_size`: dimensione file compresso
- `uncompressed_size`: dimensione non compressa
- `compression_ratio`: percentuale compressione

### queryBySourceId()

```cpp
std::optional<GaiaStar> queryBySourceId(uint64_t source_id) const;
```
**VELOCE** - Binary search O(log N).
Ritorna `std::nullopt` se non trovata.

### queryCone()

```cpp
std::vector<GaiaStar> queryCone(
    double ra, 
    double dec, 
    double radius,
    size_t max_results = 0
) const;
```
**LENTO** - Scansione lineare O(N).
- `ra`, `dec`: coordinate centro (gradi, J2016.0)
- `radius`: raggio ricerca (gradi)
- `max_results`: 0 = illimitato

### queryConeWithMagnitude()

```cpp
std::vector<GaiaStar> queryConeWithMagnitude(
    double ra, double dec, double radius,
    double mag_min, double mag_max,
    size_t max_results = 0
) const;
```
Come `queryCone()` ma con filtro magnitudine aggiuntivo.

### queryBrightest()

```cpp
std::vector<GaiaStar> queryBrightest(
    double ra, double dec, double radius,
    size_t n_brightest
) const;
```
Trova le N stelle più brillanti in una regione.
Ritorna al massimo `n_brightest` stelle, ordinate per magnitudine crescente.

### countInCone()

```cpp
size_t countInCone(double ra, double dec, double radius) const;
```
Conta stelle in una regione senza caricarle in memoria.
Più veloce di `queryCone().size()`.

### getCatalogPath()

```cpp
std::string getCatalogPath() const;
```
Ritorna il path del file catalogo.

---

## Workflow Completo

### 1. Setup Iniziale
```bash
# Verifica che GRAPPA3E sia estratto
ls ~/catalogs/GRAPPA3E/

# Compila i tool
cd /Users/michelebigi/VisualStudio\ Code/GitHub/IOC_GaiaLib/build
cmake ..
make -j4
```

### 2. Creazione Catalogo Mag 18
```bash
cd build/examples
./build_mag18_catalog ~/catalogs/GRAPPA3E gaia_mag18.cat.gz
# Attendi 30-60 minuti...
```

### 3. Test Catalogo
```bash
./test_mag18_catalog gaia_mag18.cat.gz
```

### 4. Utilizzo in Codice

```cpp
#include "ioc_gaialib/gaia_mag18_catalog.h"

int main() {
    GaiaMag18Catalog cat("gaia_mag18.cat.gz");
    
    // Query veloce per source_id
    auto star = cat.queryBySourceId(123456789);
    
    // Piccola regione: cone search OK
    auto nearby = cat.queryCone(180.0, 0.0, 0.5, 100);
    
    // Grandi regioni: meglio usare GRAPPA3E completo
    // (non usare cone search per raggio > 5°)
}
```

---

## Confronto con GRAPPA3E Completo

| Feature | Mag18 Catalog | GRAPPA3E Full |
|---------|---------------|---------------|
| **Dimensione** | ~2-3 GB | ~146 GB |
| **Stelle** | ~150M (mag ≤ 18) | 1.8B (tutte) |
| **Query source_id** | O(log N) ~1ms | O(1) ~1ms |
| **Cone search** | O(N) lento | O(tiles) veloce |
| **Parametri** | Essenziali | Completi |
| **Compressione** | gzip | No |
| **Portabilità** | 1 file | 61,202 files |

### Quando Usare Mag18 Catalog
- ✅ Query frequenti per source_id noti
- ✅ Lavoro con stelle brillanti (G ≤ 18)
- ✅ Storage limitato
- ✅ Distribuzione semplificata (1 file)
- ✅ Backup / archivio

### Quando Usare GRAPPA3E Full
- ✅ Cone search frequenti
- ✅ Stelle deboli (G > 18)
- ✅ Performance critiche
- ✅ Tutti i parametri Gaia
- ✅ Query spaziali intensive

---

## Troubleshooting

### "Failed to load catalog"
- Verifica path file corretto
- Controlla permessi lettura
- Verifica integrità file (magic number)

### Cone search troppo lenti
- Riduci raggio ricerca
- Usa `max_results` per limitare
- Considera GRAPPA3E full per query spaziali intensive

### File troppo grande
- Mag18 limita a magnitudine 18
- Per stelle più brillanti, crea versione Mag15 o Mag12
- Modifica `MAG_LIMIT` in `build_mag18_catalog.cpp`

### Out of memory durante creazione
- Riduci `reserve()` iniziale
- Processa tiles in batch più piccoli
- Chiudi altre applicazioni

---

## Estensioni Future

### Indicizzazione Spaziale
Aggiungere HEALPix index per velocizzare cone search:
```cpp
// Header esteso
struct SpatialIndex {
    uint32_t healpix_nside;
    std::vector<uint64_t> pixel_offsets;
};
```

### Parametri Addizionali
Espandere Mag18Record a 64 bytes per includere:
- Proper motion (PM RA, PM Dec)
- RUWE (quality flag)
- Photometry errors

### Multi-Threading
Parallelizzare cone search:
```cpp
std::vector<GaiaStar> queryConeParallel(
    double ra, double dec, double radius,
    size_t num_threads = 4
);
```

### Cataloghi Multiple Magnitude
- `gaia_mag12.cat.gz` - Solo stelle molto brillanti (~1M stelle)
- `gaia_mag15.cat.gz` - Stelle brillanti (~10M stelle)
- `gaia_mag18.cat.gz` - Standard (~150M stelle)
- `gaia_mag21.cat.gz` - Completo (~1.8B stelle)

---

## Files Generati

```
build/examples/
├── build_mag18_catalog      # Tool creazione catalogo
├── test_mag18_catalog        # Test suite
└── gaia_mag18.cat.gz        # Catalogo compresso (output)
```

## Integrazione con IOC_GaiaLib

Il catalogo Mag18 si integra perfettamente con le altre componenti:

```cpp
#include "ioc_gaialib/gaia_client.h"      // Query online Gaia Archive
#include "ioc_gaialib/gaia_local_catalog.h"  // GRAPPA3E completo (1.8B stelle)
#include "ioc_gaialib/gaia_mag18_catalog.h"  // Mag18 subset (150M stelle)

// Strategia ibrida
GaiaClient online;              // Fallback per dati mancanti
GaiaLocalCatalog full("...");   // Performance cone search
GaiaMag18Catalog fast("...");   // Performance source_id lookup

// Query intelligente
auto star = fast.queryBySourceId(sid);
if (!star) {
    star = full.queryBySourceId(sid);  // Stella > mag 18
}
if (!star) {
    star = online.queryBySourceId(sid);  // Stella non in GRAPPA3E
}
```

---

**Creato**: 27 novembre 2025  
**Versione**: 1.0  
**Compatibilità**: IOC_GaiaLib >= 1.0, Gaia EDR3
