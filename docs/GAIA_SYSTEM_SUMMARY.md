# IOC_GaiaLib - Gaia Catalog System Summary

## 🎯 Sistema Implementato

IOC_GaiaLib fornisce accesso completo al catalogo Gaia DR3 tramite un'architettura a più livelli ottimizzata per diversi casi d'uso.

---

## 📦 Componenti Principali

### 1. **GaiaCatalog** - API Unificata (RACCOMANDATO)
**File**: `gaia_catalog.h` / `gaia_catalog.cpp`

```cpp
#include "ioc_gaialib/gaia_catalog.h"

GaiaCatalogConfig config;
config.mag18_catalog_path = "/path/to/gaia_mag18.cat.gz";
GaiaCatalog catalog(config);

// Query automaticamente ottimizzata
auto star = catalog.queryStar(source_id);      // Usa Mag18 (veloce)
auto stars = catalog.queryCone(ra, dec, 1.0);  // Routing intelligente
```

**Caratteristiche**:
- ✅ Routing automatico verso fonte migliore
- ✅ Cache LRU 10,000 stelle
- ✅ Statistiche uso
- ✅ Thread-safe
- ✅ Fallback online (opzionale)

---

### 2. **Mag18 Catalog** - Formato Primario
**File**: `gaia_mag18_catalog.h` / `gaia_mag18_catalog.cpp`

**Dati**:
- ⭐ **303 milioni stelle** (G ≤ 18)
- 📦 **9 GB** compressed (15 GB uncompressed)
- 📁 **1 singolo file** portable
- 📊 Copre **95% use cases**

**Performance**:
```
Query source_id:  <1 ms    ✅ ECCELLENTE
Cone 0.5°:        15 sec   ⚠️  (da ottimizzare)
Cone 5°:          48 sec   ⚠️  (da ottimizzare)
```

**Generazione**:
```bash
./build_mag18_catalog ~/catalogs/GRAPPA3E ~/catalogs/gaia_mag18.cat.gz
# Tempo: 60 minuti
# Output: 9 GB
```

---

### 3. **GRAPPA3E Local Catalog** - Fonte Completa (Opzionale)
**File**: `gaia_local_catalog.h` / `gaia_local_catalog.cpp`

**Dati**:
- ⭐ **1.8 miliardi stelle** (tutte)
- 📦 **146 GB** (61,202 files)
- 🎯 Organizzazione tile 1°×1°

**Performance**:
```
Query source_id:  ~1 ms    ✅
Cone 0.3°:        1 ms     ✅ ECCELLENTE
Cone 2°:          13 ms    ✅
Box query:        1 ms     ✅
```

**Uso**: Principalmente per **generare Mag18**. Necessario solo se:
- Servono stelle deboli (G > 18)
- Cone search intensive su grandi aree

---

## 📊 Confronto Performance

| Operazione | Mag18 V1 | GRAPPA3E | Mag18 V2 (Futuro) |
|------------|----------|----------|-------------------|
| **Query source_id** | <1 ms ✅ | ~1 ms ✅ | <1 ms ✅ |
| **Cone 0.5°** | 15 sec ❌ | 1 ms ✅ | 50 ms ✅ |
| **Cone 5°** | 48 sec ❌ | 100 ms ✅ | 500 ms ✅ |
| **Storage** | 9 GB ✅ | 146 GB ❌ | 14 GB ✅ |
| **Portabilità** | 1 file ✅ | 61K files ❌ | 1 file ✅ |
| **Copertura** | 95% ✅ | 100% ✅ | 95% ✅ |

---

## 🚀 Setup Raccomandato

### Scenario 1: Uso Standard (95% progetti)

```bash
# 1. Scarica Mag18 pre-compilato (9 GB)
wget https://example.com/gaia_mag18.cat.gz

# 2. Usa GaiaCatalog
```

```cpp
#include "ioc_gaialib/gaia_catalog.h"

GaiaCatalogConfig config;
config.mag18_catalog_path = "gaia_mag18.cat.gz";
GaiaCatalog catalog(config);

// Pronto all'uso!
auto star = catalog.queryStar(12345678);
```

**Pro**:
- ✅ Setup istantaneo
- ✅ 9 GB invece di 146 GB
- ✅ Query source_id velocissime
- ✅ Sufficiente per astronomia ottica standard

---

### Scenario 2: Generazione da GRAPPA3E

Se devi generare Mag18 da zero:

```bash
# 1. Download GRAPPA3E (56 GB compressed, 146 GB extracted)
cd ~/catalogs/GRAPPA3E
# ... download archivi ...
for f in *.7z; do 7z x "$f"; done

# 2. Genera Mag18 (60 minuti)
cd IOC_GaiaLib/build/examples
./build_mag18_catalog ~/catalogs/GRAPPA3E ~/catalogs/gaia_mag18.cat.gz

# 3. Usa Mag18
./test_unified_catalog ~/catalogs/gaia_mag18.cat.gz
```

---

### Scenario 3: Uso Avanzato (con GRAPPA3E)

Solo se necessiti:
- Stelle deboli (G > 18)
- Cone search intensive

```cpp
GaiaCatalogConfig config;
config.mag18_catalog_path = "gaia_mag18.cat.gz";
config.grappa_catalog_path = "~/catalogs/GRAPPA3E";  // Opzionale
config.prefer_grappa_for_cone = true;  // Usa GRAPPA per cone

GaiaCatalog catalog(config);
// Routing automatico verso fonte ottimale
```

---

## 🔧 Esempi d'Uso

### Query Singola Stella
```cpp
GaiaCatalog catalog(config);

// Binary search - <1 ms
auto star = catalog.queryStar(12345678901234);
if (star) {
    std::cout << "RA: " << star->ra 
              << " Dec: " << star->dec 
              << " G: " << star->phot_g_mean_mag << "\n";
}
```

### Cone Search
```cpp
// Cerca in 1° di raggio
auto stars = catalog.queryCone(
    180.0,  // RA
    0.0,    // Dec
    1.0,    // radius (degrees)
    100     // max results
);

for (const auto& s : stars) {
    std::cout << "source_id: " << s.source_id 
              << " G=" << s.phot_g_mean_mag << "\n";
}
```

### Filtro Magnitudine
```cpp
// Stelle brillanti 10 < G < 12
auto bright = catalog.queryConeWithMagnitude(
    180.0, 0.0, 2.0,  // posizione e raggio
    10.0, 12.0,       // mag range
    50                // max results
);
```

### Stelle Più Brillanti
```cpp
// 10 stelle più brillanti in regione
auto top10 = catalog.queryBrightest(
    180.0, 0.0,  // posizione
    5.0,         // raggio
    10           // numero stelle
);

// Ordinate per magnitudine crescente
for (size_t i = 0; i < top10.size(); ++i) {
    std::cout << (i+1) << ". G=" << top10[i].phot_g_mean_mag << "\n";
}
```

### Batch Query
```cpp
std::vector<uint64_t> source_ids = {123, 456, 789, ...};

// Query multipla ottimizzata
auto results = catalog.queryStars(source_ids);

for (const auto& [sid, star] : results) {
    std::cout << "Found: " << sid << "\n";
}
```

### Statistiche
```cpp
auto stats = catalog.getStatistics();

std::cout << "Total queries: " << stats.total_queries << "\n";
std::cout << "Mag18 queries: " << stats.mag18_queries << "\n";
std::cout << "Cache hits: " << stats.cache_hits << "\n";

auto [hits, misses] = catalog.getCacheStats();
double hit_rate = 100.0 * hits / (hits + misses);
std::cout << "Cache hit rate: " << hit_rate << "%\n";
```

---

## 📁 Struttura Files

```
IOC_GaiaLib/
├── include/ioc_gaialib/
│   ├── gaia_catalog.h           # ⭐ API unificata (usa questa!)
│   ├── gaia_mag18_catalog.h     # Catalogo Mag18
│   ├── gaia_local_catalog.h     # GRAPPA3E full
│   └── types.h                  # Tipi comuni
│
├── src/
│   ├── gaia_catalog.cpp
│   ├── gaia_mag18_catalog.cpp
│   └── gaia_local_catalog.cpp
│
├── examples/
│   ├── test_unified_catalog.cpp    # ⭐ Test API unificata
│   ├── test_mag18_catalog.cpp      # Test Mag18
│   ├── build_mag18_catalog.cpp     # Genera Mag18
│   └── comprehensive_local_test.cpp # Test GRAPPA3E
│
├── docs/
│   ├── GAIA_MAG18_CATALOG.md       # Guida Mag18
│   ├── MAG18_IMPROVEMENTS.md       # Analisi ottimizzazioni
│   ├── MAG18_QUICKSTART.md         # Quick start
│   └── GRAPPA3E_IMPLEMENTATION.md  # Documentazione completa
│
└── Data/ (non in repo)
    ├── gaia_mag18.cat.gz           # 9 GB - FORMATO PRIMARIO
    └── GRAPPA3E/                   # 146 GB - Opzionale
        ├── 5/...                   # 170 directories
        └── 174/...                 # 61,202 files totali
```

---

## 🎯 Decisioni Architetturali

### Perché Mag18 come Primario?

1. **Copertura**: 95% stelle osservabili (G ≤ 18)
2. **Storage**: 94% risparmio (9 GB vs 146 GB)
3. **Portabilità**: 1 file vs 61,202 files
4. **Performance source_id**: <1 ms (eccellente)
5. **Distribuzione**: Download veloce, setup immediato

### Quando Serve GRAPPA3E?

Solo per:
- Stelle molto deboli (G > 18) - raro
- Cone search intensive su grandi aree
- Validazione scientifica completa

### Routing Intelligente GaiaCatalog

```
Query source_id → Mag18 (binary search)
Cone small (<5°) → Mag18 (se configurato)
Cone large (≥5°) → GRAPPA3E (se disponibile)
Mag > 18 → GRAPPA3E (unica fonte)
Fallback → Online Gaia Archive (opzionale)
```

---

## 🔮 Roadmap Futuri Miglioramenti

### Mag18 V2 (Pianificato)
Vedere `docs/MAG18_IMPROVEMENTS.md` per dettagli completi.

**Priorità 1**: HEALPix Spatial Index
- Cone search: da 15s a 50ms (300x più veloce!)
- Implementazione: 2-3 giorni
- Impatto: **CRITICO**

**Priorità 2**: Chunk Compression
- Ulteriore 2-5x speedup
- Cache efficiente chunk decompressi

**Priorità 3**: Record Esteso (80 bytes)
- Proper motion (PM RA, PM Dec)
- Errori fotometrici completi
- RUWE (quality indicator)

**Target V2**:
```
Size: 14 GB (+55%)
Cone 0.5°: 50 ms (300x più veloce)
Cone 5°: 500 ms (96x più veloce)
Parametri: 13 campi invece di 7
```

---

## 📚 Documentazione

- **`GAIA_MAG18_CATALOG.md`** - Guida completa Mag18
- **`MAG18_IMPROVEMENTS.md`** - Analisi ottimizzazioni dettagliata
- **`MAG18_QUICKSTART.md`** - Setup rapido
- **`GRAPPA3E_IMPLEMENTATION.md`** - Implementazione GRAPPA3E
- **Questo file** - Overview sistema completo

---

## ✅ Checklist Uso

### Nuovo Progetto
- [ ] Scarica/genera `gaia_mag18.cat.gz` (9 GB)
- [ ] Compila IOC_GaiaLib
- [ ] Usa `GaiaCatalog` con Mag18
- [ ] Verifica copertura magnitudine (95% casi OK con G ≤ 18)
- [ ] Opzionale: Aggiungi GRAPPA3E se necessario

### Performance Check
- [ ] Query source_id < 1 ms? ✅
- [ ] Cone search OK per tue esigenze? (considera V2 se lento)
- [ ] Cache hit rate > 50%? (buono)
- [ ] Statistiche uso controllate?

### Produzione
- [ ] Mag18 accessibile da percorso stabile
- [ ] Log errori configurato
- [ ] Monitoring performance attivo
- [ ] Backup Mag18 disponibile
- [ ] Documentazione API per utenti

---

## 🎓 Conclusioni

**IOC_GaiaLib offre ora un sistema Gaia completo e production-ready**:

✅ **API semplice e unificata** (`GaiaCatalog`)  
✅ **Formato primario compatto** (Mag18 - 9 GB)  
✅ **Performance eccellenti** per source_id (<1 ms)  
✅ **Copertura 95% use cases** (G ≤ 18)  
✅ **Opzionale: Catalogo completo** (GRAPPA3E - 146 GB)  
✅ **Routing intelligente** automatico  
✅ **Roadmap chiara** per ottimizzazioni future  

**Raccomandazione**: Inizia con Mag18. Aggiungi GRAPPA3E solo se necessario.

---

**Versione**: 1.0  
**Data**: 27 novembre 2025  
**Status**: ✅ Production Ready
