# IOC_GaiaLib - API Guide

**Interfaccia Unificata per Catalogo Gaia DR3**

## 🎯 Filosofia

IOC_GaiaLib fornisce un'**interfaccia trasparente e unificata** per accedere al catalogo Gaia DR3. Non devi preoccuparti del formato del catalogo sottostante: la libreria gestisce tutto automaticamente.

---

## 🚀 Quick Start

### 1. Include Header

```cpp
#include <ioc_gaialib/unified_gaia_catalog.h>

using namespace ioc::gaia;
```

### 2. Inizializza Catalogo

```cpp
int main() {
    // Path al catalogo
    std::string home = getenv("HOME");
    
    // Configurazione JSON
    std::string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
    })";
    
    // Inizializza (una sola volta)
    if (!UnifiedGaiaCatalog::initialize(config)) {
        std::cerr << "Errore inizializzazione catalogo\n";
        return 1;
    }
    
    // Ottieni istanza singleton
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // ... usa il catalogo ...
    
    // Cleanup (alla fine del programma)
    UnifiedGaiaCatalog::shutdown();
    return 0;
}
```

---

## 📊 Strutture Dati

### GaiaStar - Dati Stellari

```cpp
struct GaiaStar {
    int64_t source_id;           // Gaia DR3 source identifier
    
    // Astrometria (ICRS, Epoch 2016.0)
    double ra;                   // Right Ascension [degrees]
    double dec;                  // Declination [degrees]
    double parallax;             // Parallax [mas]
    double parallax_error;       // Parallax error [mas]
    
    // Moto proprio
    double pmra;                 // PM in RA * cos(dec) [mas/yr]
    double pmdec;                // PM in Dec [mas/yr]
    double pmra_error;           // PM RA error [mas/yr]
    double pmdec_error;          // PM Dec error [mas/yr]
    
    // Fotometria
    double phot_g_mean_mag;      // G-band magnitude [mag]
    double phot_bp_mean_mag;     // BP-band magnitude [mag]
    double phot_rp_mean_mag;     // RP-band magnitude [mag]
    double bp_rp;                // BP-RP color [mag]
    
    // Qualità
    double astrometric_excess_noise;
    double astrometric_chi2_al;
    int visibility_periods_used;
    double ruwe;                 // Renormalized Unit Weight Error
    
    // Cross-match
    std::string sao_designation;
    std::string tycho2_designation;
    std::string hd_designation;
    std::string hip_designation;
    std::string common_name;
};
```

### QueryParams - Parametri Cone Search

```cpp
struct QueryParams {
    double ra_center;            // Centro RA [degrees]
    double dec_center;           // Centro Dec [degrees]
    double radius;               // Raggio ricerca [degrees]
    double max_magnitude;        // Magnitudine G massima
    double min_parallax;         // Parallasse minima [mas] (-1 = no limit)
    
    QueryParams() 
        : ra_center(0.0), dec_center(0.0), radius(1.0), 
          max_magnitude(20.0), min_parallax(-1.0) {}
};
```

### CorridorQueryParams - Parametri Corridor Search

```cpp
struct CelestialPoint {
    double ra;                   // Right Ascension [degrees]
    double dec;                  // Declination [degrees]
};

struct CorridorQueryParams {
    std::vector<CelestialPoint> path;  // Punti del percorso (minimo 2)
    double width;                       // Semi-larghezza corridoio [degrees]
    double max_magnitude;               // Magnitudine G massima
    double min_parallax;                // Parallasse minima [mas]
    size_t max_results;                 // Limite risultati (0 = illimitato)
    
    CorridorQueryParams() 
        : width(0.5), max_magnitude(20.0), 
          min_parallax(-1.0), max_results(0) {}
    
    // Parsing JSON
    static CorridorQueryParams fromJSON(const std::string& json);
    std::string toJSON() const;
    
    // Utilità
    bool isValid() const;
    double getPathLength() const;  // Lunghezza totale path [degrees]
};
```

---

## 🔍 API Reference

### Inizializzazione

```cpp
// Inizializza catalogo (chiamare una sola volta)
static bool initialize(const std::string& json_config);

// Ottieni istanza singleton
static UnifiedGaiaCatalog& getInstance();

// Verifica se inizializzato
bool isInitialized() const;

// Ottieni info catalogo
CatalogInfo getCatalogInfo() const;

// Cleanup (alla fine del programma)
static void shutdown();
```

### Query Spaziali

#### Cone Search

Cerca stelle in un cono centrato su coordinate specifiche:

```cpp
std::vector<GaiaStar> queryCone(const QueryParams& params) const;
```

**Esempio:**
```cpp
QueryParams params;
params.ra_center = 83.0;      // Orione
params.dec_center = -5.0;
params.radius = 2.0;          // 2° di raggio
params.max_magnitude = 10.0;  // Solo stelle luminose

auto stars = catalog.queryCone(params);
```

#### Corridor Search

Cerca stelle lungo un percorso con larghezza specificata:

```cpp
std::vector<GaiaStar> queryCorridor(const CorridorQueryParams& params) const;
std::vector<GaiaStar> queryCorridorJSON(const std::string& json_params) const;
```

**Esempio:**
```cpp
CorridorQueryParams params;
params.path = {
    {73.0, 20.0},   // Punto iniziale
    {75.0, 22.0},   // Punto intermedio
    {77.0, 20.0}    // Punto finale
};
params.width = 0.5;           // 0.5° semi-larghezza (1° totale)
params.max_magnitude = 14.0;
params.max_results = 1000;

auto stars = catalog.queryCorridor(params);
```

**Esempio JSON:**
```cpp
std::string json = R"({
    "path": [
        {"ra": 73.0, "dec": 20.0},
        {"ra": 75.0, "dec": 22.0}
    ],
    "width": 0.5,
    "max_magnitude": 14.0,
    "min_parallax": 0.0,
    "max_results": 1000
})";

auto stars = catalog.queryCorridorJSON(json);
```

### Query per Identificativo

#### Source ID

```cpp
std::optional<GaiaStar> queryBySourceId(uint64_t source_id) const;
```

**Esempio:**
```cpp
auto star = catalog.queryBySourceId(3411546266140512128ULL);
if (star) {
    std::cout << "RA=" << star->ra << " Dec=" << star->dec << "\n";
}
```

#### Nome Comune (IAU)

```cpp
std::optional<GaiaStar> queryByName(const std::string& name) const;
```

**Esempio:**
```cpp
auto sirius = catalog.queryByName("Sirius");
auto vega = catalog.queryByName("Vega");
auto polaris = catalog.queryByName("Polaris");
```

#### Cataloghi Cross-Match

```cpp
std::optional<GaiaStar> queryByHD(const std::string& hd_number) const;
std::optional<GaiaStar> queryByHipparcos(const std::string& hip_number) const;
std::optional<GaiaStar> queryBySAO(const std::string& sao_number) const;
std::optional<GaiaStar> queryByTycho2(const std::string& tycho2_designation) const;
```

**Esempio:**
```cpp
auto star1 = catalog.queryByHD("48915");        // Sirius (Henry Draper)
auto star2 = catalog.queryByHipparcos("32349"); // Sirius (Hipparcos)
auto star3 = catalog.queryBySAO("151881");      // Sirius (SAO)
```

### Operazioni Asincrone

```cpp
// Cone search asincrona
std::future<std::vector<GaiaStar>> queryAsync(
    const QueryParams& params,
    ProgressCallback progress_callback = nullptr
) const;

// Corridor search asincrona
std::future<std::vector<GaiaStar>> queryCorridorAsync(
    const CorridorQueryParams& params,
    ProgressCallback progress_callback = nullptr
) const;
```

**Esempio:**
```cpp
auto progress = [](int percent, const std::string& status) {
    std::cout << "Progress: " << percent << "% - " << status << "\n";
};

auto future = catalog.queryAsync(params, progress);

// Fai altre cose mentre la query è in esecuzione...

auto stars = future.get();  // Ottieni risultati quando pronti
```

### Batch Queries

```cpp
std::vector<std::vector<GaiaStar>> batchQuery(
    const std::vector<QueryParams>& param_list
) const;
```

**Esempio:**
```cpp
std::vector<QueryParams> queries;
for (int i = 0; i < 10; i++) {
    QueryParams p;
    p.ra_center = i * 10.0;
    p.dec_center = i * 5.0;
    p.radius = 1.0;
    queries.push_back(p);
}

auto results = catalog.batchQuery(queries);
// results[i] contiene stelle per queries[i]
```

### Monitoring

```cpp
// Ottieni statistiche performance
CatalogStats getStatistics() const;

// Pulisci cache
void clearCache();

// Riconfigura catalogo
bool reconfigure(const std::string& json_config);
```

---

## ⚡ Performance

| Query Type | Tempo | Stelle | Note |
|------------|-------|--------|------|
| Cone 0.5° | **0.001 ms** | ~50 | Raggio piccolo |
| Cone 5° | **13 ms** | ~500 | Raggio medio |
| Cone 15° | **18 ms** | ~5,000 | Raggio grande |
| Corridor 5°×0.2° | **40 ms** | ~1,000 | Percorso semplice |
| Corridor L-shape | **37 ms** | ~133 | Multi-segmento |
| By source_id | **<1 ms** | 1 | Ricerca diretta |
| By name | **<0.1 ms** | 1 | Cache IAU |

---

## 📁 Configurazione Catalogo

### Path Catalogo

Il catalogo deve trovarsi in:
```
~/.catalog/gaia_mag18_v2_multifile/
├── metadata.dat        # 185 KB - Header + indice HEALPix
└── chunks/             # 19 GB - Dati stelle
    ├── chunk_000.dat   # 1M stelle ciascuno
    ├── chunk_001.dat
    └── ... (232 chunks)
```

### JSON Configurazione

```json
{
    "catalog_type": "multifile_v2",
    "multifile_directory": "~/.catalog/gaia_mag18_v2_multifile"
}
```

**Required parameters:**
- `catalog_type`: Must be `"multifile_v2"`
- `multifile_directory`: Path to catalog directory

**Path dinamico in C++:**
```cpp
std::string home = getenv("HOME");
std::string config = R"({
    "catalog_type": "multifile_v2",
    "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
})";
```

---

## 💡 Best Practices

### 1. Inizializza Una Sola Volta

```cpp
// ✅ BUONO: Inizializza all'inizio del programma
int main() {
    UnifiedGaiaCatalog::initialize(config);
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Usa catalog per tutte le query
    for (int i = 0; i < 1000; i++) {
        catalog.queryCone(params);  // Riusa lo stesso catalogo
    }
}

// ❌ MALE: Non inizializzare ripetutamente
for (int i = 0; i < 1000; i++) {
    UnifiedGaiaCatalog::initialize(config);  // ❌ LENTISSIMO!
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    catalog.queryCone(params);
}
```

### 2. Usa Path Dinamici

```cpp
// ✅ BUONO: Path dinamico con getenv
std::string home = getenv("HOME");
std::string path = home + "/.catalog/gaia_mag18_v2_multifile";

// ❌ MALE: Path hardcoded
std::string path = "/Users/utente/.catalog/gaia_mag18_v2_multifile";
```

### 3. Gestisci Errori

```cpp
auto star = catalog.queryBySourceId(source_id);
if (star) {
    // Stella trovata
    std::cout << "RA=" << star->ra << "\n";
} else {
    // Stella non trovata
    std::cout << "Stella non nel catalogo\n";
}
```

### 4. Usa Filtri per Performance

```cpp
QueryParams params;
params.ra_center = 180.0;
params.dec_center = 0.0;
params.radius = 10.0;
params.max_magnitude = 12.0;  // ✅ Filtra durante la query
params.min_parallax = 0.5;    // ✅ Più veloce che filtrare dopo

auto stars = catalog.queryCone(params);
```

---

## 🔧 Esempi Completi

### Esempio 1: Ricerca Semplice

```cpp
#include <ioc_gaialib/unified_gaia_catalog.h>
#include <iostream>

int main() {
    using namespace ioc::gaia;
    
    // Inizializza
    std::string home = getenv("HOME");
    std::string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
    })";
    
    if (!UnifiedGaiaCatalog::initialize(config)) {
        std::cerr << "Errore inizializzazione\n";
        return 1;
    }
    
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Query
    QueryParams params;
    params.ra_center = 83.0;
    params.dec_center = -5.0;
    params.radius = 2.0;
    params.max_magnitude = 10.0;
    
    auto stars = catalog.queryCone(params);
    
    std::cout << "Trovate " << stars.size() << " stelle\n";
    
    for (const auto& star : stars) {
        std::cout << "Gaia " << star.source_id 
                  << " | RA=" << star.ra 
                  << " Dec=" << star.dec
                  << " | Mag=" << star.phot_g_mean_mag << "\n";
    }
    
    UnifiedGaiaCatalog::shutdown();
    return 0;
}
```

### Esempio 2: Corridor Query

```cpp
#include <ioc_gaialib/unified_gaia_catalog.h>
#include <iostream>

int main() {
    using namespace ioc::gaia;
    
    // Inizializza catalogo
    std::string home = getenv("HOME");
    std::string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
    })";
    
    UnifiedGaiaCatalog::initialize(config);
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Definisci corridoio (es. path di occultazione)
    CorridorQueryParams params;
    params.path = {
        {73.0, 20.0},   // Start
        {75.0, 22.0},   // Middle
        {77.0, 20.0}    // End
    };
    params.width = 0.3;           // 0.3° semi-larghezza
    params.max_magnitude = 12.0;  // Stelle luminose
    params.max_results = 500;
    
    std::cout << "Lunghezza percorso: " << params.getPathLength() << "°\n";
    std::cout << "Larghezza totale: " << params.width * 2 << "°\n";
    
    auto stars = catalog.queryCorridor(params);
    
    std::cout << "Trovate " << stars.size() << " stelle nel corridoio\n";
    
    UnifiedGaiaCatalog::shutdown();
    return 0;
}
```

### Esempio 3: Query per Nome

```cpp
#include <ioc_gaialib/unified_gaia_catalog.h>
#include <iostream>

int main() {
    using namespace ioc::gaia;
    
    // Inizializza
    std::string home = getenv("HOME");
    std::string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
    })";
    
    UnifiedGaiaCatalog::initialize(config);
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // Query stelle famose
    std::vector<std::string> names = {
        "Sirius", "Vega", "Polaris", "Rigel", "Betelgeuse"
    };
    
    for (const auto& name : names) {
        auto star = catalog.queryByName(name);
        if (star) {
            std::cout << name << ": "
                      << "RA=" << star->ra << " "
                      << "Dec=" << star->dec << " "
                      << "Mag=" << star->phot_g_mean_mag << "\n";
        }
    }
    
    UnifiedGaiaCatalog::shutdown();
    return 0;
}
```

---

## 📞 Supporto

- **Repository**: https://github.com/manvalan/IOC_GaiaLib
- **Issues**: https://github.com/manvalan/IOC_GaiaLib/issues
- **Documentation**: `/docs/` directory

---

## 🔗 File Correlati

- `include/ioc_gaialib/unified_gaia_catalog.h` - Header principale
- `include/ioc_gaialib/types.h` - Strutture dati
- `src/unified_gaia_catalog.cpp` - Implementazione
- `examples/unified_api_demo.cpp` - Demo completa
