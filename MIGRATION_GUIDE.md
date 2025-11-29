# IOC GaiaLib Migration Guide 🔄

## From V2 APIs to Unified Catalog System

**Questa guida spiega come migrare dalle API V2 deprecate all'API UnifiedGaiaCatalog raccomandata.**

---

## 📍 Catalogo e Performance

**Catalogo Multifile V2** - Unico formato supportato

| Percorso | Performance | Caratteristiche |
|----------|-------------|-----------------|
| `~/.catalog/gaia_mag18_v2_multifile/` | ⭐ **0.001ms - 18ms** | 231M stelle, indice HEALPix, 19 GB |

> **Nota:** La libreria gestisce automaticamente il formato del catalogo. Specifica solo il path.

---

### ⚠️ API Deprecate (Da Non Usare)
```cpp
// ❌ DEPRECATO - Non utilizzare in nuovo codice
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include "ioc_gaialib/multifile_catalog_v2.h" 
#include "ioc_gaialib/concurrent_multifile_catalog_v2.h"

// ❌ Codice vecchio
Mag18CatalogV2 catalog("path/to/catalog.mag18v2");
MultiFileCatalogV2 multifile("path/to/multifile_v2/");
```

### ✅ API Raccomandata: UnifiedGaiaCatalog

```cpp
#include "ioc_gaialib/unified_gaia_catalog.h"
#include <cstdlib>

using namespace ioc::gaia;

int main() {
    // Configurazione JSON con path dinamico
    std::string home = std::getenv("HOME");
    std::string config = R"({
        "catalog_type": "multifile_v2",
        "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
    })";

    // Inizializzazione (una volta sola)
    if (!UnifiedGaiaCatalog::initialize(config)) {
        std::cerr << "Errore inizializzazione catalogo" << std::endl;
        return 1;
    }

    // Ottieni istanza singleton
    auto& catalog = UnifiedGaiaCatalog::getInstance();
    
    // === CONE SEARCH ===
    ConeSearchParams params{180.0, 0.0, 5.0};  // RA, Dec, radius (gradi)
    params.max_magnitude = 12.0;
    auto stars = catalog.queryCone(params);
    std::cout << "Trovate " << stars.size() << " stelle\n";
    
    // === QUERY PER NOME (451 stelle IAU ufficiali) ===
    auto sirius = catalog.queryByName("Sirius");
    auto vega = catalog.queryByName("Vega");
    auto polaris = catalog.queryByName("Polaris");
    auto rigel = catalog.queryByName("Rigel");
    auto betelgeuse = catalog.queryByName("Betelgeuse");
    
    if (sirius) {
        std::cout << "Sirius: RA=" << sirius->ra 
                  << " Dec=" << sirius->dec 
                  << " mag=" << sirius->phot_g_mean_mag << "\n";
    }
    
    // === QUERY PER DESIGNAZIONE ===
    auto byHD = catalog.queryByName("HD 48915");     // Sirius (Henry Draper)
    auto byHIP = catalog.queryByName("HIP 32349");   // Sirius (Hipparcos)
    auto byBayer = catalog.queryByName("α CMa");     // Sirius (Bayer)
    
    // === STELLE PIÙ LUMINOSE ===
    auto brightest = catalog.queryBrightest(params, 10);
    
    return 0;
}

```

---

### 📁 Formato Catalogo

La libreria usa il formato **multifile V2** - una directory con file chunk HEALPix ottimizzati:

- **Performance**: 0.001-18ms per query
- **Dimensione**: 19 GB
- **Stelle**: 231 milioni (mag ≤ 18)
- **Indice**: HEALPix NSIDE=64 per ricerca spaziale veloce

---

### 🔄 Migrazione Step-by-Step

#### 1. Aggiorna gli Include
```cpp
// Prima (DEPRECATO)
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"

// Dopo (RACCOMANDATO)
#include "ioc_gaialib/unified_gaia_catalog.h"
```

#### 2. Sostituisci l'Inizializzazione
```cpp
// Prima (DEPRECATO)
Mag18CatalogV2 catalog;
if (!catalog.open("/path/file.mag18v2")) {
    // errore
}

// Dopo (RACCOMANDATO)
std::string home = std::getenv("HOME");
std::string config = R"({
    "catalog_type": "multifile_v2",
    "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
})";

if (!UnifiedGaiaCatalog::initialize(config)) {
    // errore
}
auto& catalog = UnifiedGaiaCatalog::getInstance();
```

#### 3. Aggiorna le Query
```cpp
// Prima (V2 - DEPRECATO)
auto records = catalog.queryCone(ra, dec, radius);
for (const auto& record : records) {
    GaiaStar star = record.toGaiaStar();
    // usa star
}

// Dopo (Unified - RACCOMANDATO)
ConeSearchParams params{ra, dec, radius};
params.max_magnitude = 15.0;
auto stars = catalog.queryCone(params);
for (const auto& star : stars) {
    // usa star direttamente
}
```

### 🚀 Vantaggi della Migrazione

| Feature | API V2 (Deprecata) | UnifiedGaiaCatalog |
|---------|-------------------|---------------------|
| Nomi IAU | ❌ Non disponibile | ✅ 451 stelle ufficiali |
| Performance multifile | ~50ms | ✅ **0.001-18ms** |
| Cross-match HD/HIP | ❌ Manuale | ✅ Automatico |
| Designazioni Bayer | ❌ Non disponibile | ✅ 297 supportate |
| Configurazione | Hardcoded | ✅ JSON flessibile |
| Singleton thread-safe | ❌ | ✅ |
| **Corridor Query** | ❌ Non disponibile | ✅ **Polyline search** |

---

## 🛤️ Nuova Feature: Corridor Query

Trova stelle lungo un percorso definito da punti multipli. Ideale per:
- Predizioni di occultazioni
- Tracce di satelliti
- Attraversamenti eclittici

### Uso Base

```cpp
#include "ioc_gaialib/unified_gaia_catalog.h"

// Definisci un corridoio lungo il percorso di occultazione
ioc::gaia::CorridorQueryParams params;
params.path = {
    {73.0, 20.0},   // Punto iniziale (RA, Dec in gradi)
    {75.0, 22.0},   // Punto intermedio
    {77.0, 20.0}    // Punto finale
};
params.width = 0.5;           // Semi-larghezza in gradi
params.max_magnitude = 14.0;  // Magnitudine massima
params.max_results = 1000;    // Limite risultati

auto& catalog = UnifiedGaiaCatalog::getInstance();
auto stars = catalog.queryCorridor(params);
```

### Uso con JSON

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

### Performance

| Lunghezza Percorso | Larghezza | Tempo | Note |
|-------------------|-----------|-------|------|
| 2° | 0.2° | ~40 ms | Percorso semplice |
| 5° | 0.5° | ~100 ms | Corridoio medio |
| L-shaped | 0.15° | ~40 ms | Percorso multi-segmento |

---

### 🔧 Configurazione JSON

```json
{
    "catalog_type": "multifile_v2",
    "multifile_directory": "~/.catalog/gaia_mag18_v2_multifile"
}
```

Oppure con path dinamico in C++:

```cpp
std::string home = std::getenv("HOME");
std::string config = R"({
    "catalog_type": "multifile_v2",
    "multifile_directory": ")" + home + R"(/.catalog/gaia_mag18_v2_multifile"
})";
```

---

### ❗ Note Importanti

1. **Path dinamici**: Usa `getenv("HOME")` invece di path hardcoded
2. **Interfaccia trasparente**: La libreria gestisce automaticamente il formato del catalogo
3. **Singleton pattern**: Inizializza una volta con `initialize()`, poi usa `getInstance()`
4. **Nomi IAU**: 451 stelle ufficiali supportate nativamente

---

### 🆘 Risoluzione Problemi

**Errore: "Catalog not initialized"**
```cpp
// ✅ Soluzione: Chiama initialize() prima di getInstance()
UnifiedGaiaCatalog::initialize(config);
auto& catalog = UnifiedGaiaCatalog::getInstance();
```

**Errore: "Catalog path not found"**
```cpp
// ✅ Soluzione: Verifica che il path esista
std::string home = std::getenv("HOME");
std::string path = home + "/.catalog/gaia_mag18_v2_multifile";
// Verifica: ls -la ~/.catalog/gaia_mag18_v2_multifile/
```

**queryByName() restituisce nullptr**
```cpp
// ✅ Soluzione: Verifica che il nome sia corretto e sia tra i 451 nomi IAU supportati
auto star = catalog.queryByName("Sirius");  // ✅
auto star = catalog.queryByName("sirius");  // ❌ case-sensitive
```

**Performance lente**
```cpp
// ✅ Soluzione: Verifica che il path del catalogo sia corretto
"catalog_type": "multifile_v2",
"multifile_directory": "/Users/user/.catalog/gaia_mag18_v2_multifile"
// Verifica che metadata.dat e chunks/ esistano
```

---

### 📞 Supporto

Per esempi completi, consulta:
- `examples/unified_api_demo.cpp` - Demo API unificata
- `examples/complete_performance_benchmark.cpp` - Benchmark performance
- `examples/iau_names_demo.cpp` - Demo nomi IAU