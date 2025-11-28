# IOC GaiaLib Migration Guide 🔄

## From V2 APIs to Unified Catalog System

**Se la tua app cerca ancora cataloghi V2, segui questa guida per la migrazione.**

### ⚠️ API Deprecate (Da Non Usare)
```cpp
// ❌ DEPRECATO - Non utilizzare
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include "ioc_gaialib/multifile_catalog_v2.h" 
#include "ioc_gaialib/concurrent_multifile_catalog_v2.h"

// ❌ Codice vecchio
Mag18CatalogV2 catalog("path/to/catalog.mag18v2");
MultiFileCatalogV2 multifile("path/to/multifile_v2/");
```

### ✅ Nuove API (Raccomandato)

#### Opzione 1: UnifiedGaiaCatalog (Più Semplice)
```cpp
// ✅ NUOVO - Uso raccomandato
#include "ioc_gaialib/unified_gaia_catalog.h"

// Configurazione JSON
std::string config = R"({
    "catalog_type": "compressed_v2",
    "compressed_file_path": "/path/to/new_catalog.gz",
    "timeout_seconds": 30
})";

// Inizializzazione
if (!UnifiedGaiaCatalog::initialize(config)) {
    std::cerr << "Errore inizializzazione" << std::endl;
    return false;
}

// Uso
auto& catalog = UnifiedGaiaCatalog::getInstance();
auto stars = catalog.queryCone({83.82, 22.01, 2.0, 18.0, 0});
auto sirius = catalog.queryByName("Sirius");
```

#### Opzione 2: GaiaCatalog (Più Controllo)
```cpp
// ✅ NUOVO - API avanzata
#include "ioc_gaialib/gaia_catalog.h"

GaiaCatalogConfig config;
config.mag18_catalog_path = "/path/to/new_catalog.gz";
config.grappa_catalog_path = "/path/to/grappa/";
config.enable_online_fallback = true;

GaiaCatalog catalog(config);

// Query automatiche (scelta ottimale del catalogo)
auto star = catalog.queryStar(12345678901234);      // Source ID
auto stars = catalog.queryCone(180.0, 0.0, 1.0);   // Cone search
auto bright = catalog.queryBrightest(180, 0, 5, 10); // Top 10 brightest
```

### 📁 Nuovi Formati Catalogo

| Formato Vecchio | Formato Nuovo | Descrizione |
|-----------------|---------------|-------------|
| `*.mag18v2` | `*.gz` (GaiaMag18Catalog) | Catalogo compresso mag≤18 |
| `multifile_v2/` | GRAPPA3E directory | Catalogo completo locale |
| Online V2 | Unified online | Query ESA automatiche |

### 🔄 Migrazione Step-by-Step

#### 1. Aggiorna gli Include
```cpp
// Prima
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"

// Dopo  
#include "ioc_gaialib/unified_gaia_catalog.h"
```

#### 2. Sostituisci l'Inizializzazione
```cpp
// Prima
Mag18CatalogV2 catalog;
if (!catalog.open("/path/file.mag18v2")) {
    // errore
}

// Dopo
std::string config = R"({
    "catalog_type": "compressed_v2", 
    "compressed_file_path": "/path/file.gz"
})";

if (!UnifiedGaiaCatalog::initialize(config)) {
    // errore
}
auto& catalog = UnifiedGaiaCatalog::getInstance();
```

#### 3. Aggiorna le Query
```cpp
// Prima (V2)
auto records = catalog.queryCone(ra, dec, radius);
for (const auto& record : records) {
    GaiaStar star = record.toGaiaStar();
    // usa star
}

// Dopo (Unified)
auto stars = catalog.queryCone({ra, dec, radius});
for (const auto& star : stars) {
    // usa star direttamente
}
```

### 🚀 Vantaggi della Migrazione

1. **Nomi Stelle**: Integrazione IAU automatica (451 stelle ufficiali)
2. **Performance**: Selezione automatica del catalogo ottimale  
3. **Cross-match**: HD, HIP, Bayer, Flamsteed automatici
4. **Online Fallback**: Query ESA automatiche se mancano dati
5. **Future-proof**: API stabile per versioni future

### 🔧 Configurazioni Esempio

#### Per Catalogo Locale Compresso
```json
{
    "catalog_type": "compressed_v2",
    "compressed_file_path": "~/catalogs/gaia_mag18_new.gz",
    "timeout_seconds": 10  
}
```

#### Per Catalogo Completo GRAPPA3E
```json
{
    "catalog_type": "multifile_v2", 
    "multifile_directory": "~/catalogs/grappa3e/",
    "max_cached_chunks": 100
}
```

#### Per Solo Online
```json
{
    "catalog_type": "online_esa",
    "timeout_seconds": 30
}
```

### ❗ Note Importanti

1. **File V2 non supportati**: I nuovi cataloghi usano formati diversi
2. **Percorsi diversi**: Aggiorna i percorsi nei file di configurazione  
3. **Testing**: Testa accuratamente dopo la migrazione
4. **Backward compatibility**: Le API V2 funzionano ancora ma sono deprecate

### 🆘 Risoluzione Problemi

**Errore: "cerca di aprire catalogo v2"**
- ✅ Aggiorna gli include alle nuove API
- ✅ Cambia percorsi da `.mag18v2` a `.gz` 
- ✅ Usa `UnifiedGaiaCatalog` invece di `Mag18CatalogV2`

**Errore: "file non trovato"**  
- ✅ Verifica il nuovo percorso del catalogo
- ✅ Usa configurazione JSON per specificare i percorsi

**Performance peggiori**
- ✅ La migrazione migliora le performance grazie alla selezione automatica
- ✅ Configura correttamente il tipo di catalogo per il tuo uso

### 📞 Supporto

Per domande sulla migrazione, consulta gli esempi in `examples/migration_guide.cpp` e `examples/unified_api_demo.cpp`.