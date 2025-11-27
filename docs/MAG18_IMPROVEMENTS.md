# Gaia Mag18 Catalog - Analisi e Miglioramenti

## Analisi Performance Corrente

### ✅ Punti di Forza
1. **Query per source_id**: Eccellente - Binary search O(log N), <1 ms
2. **Compressione**: 38.5% (9 GB vs 15 GB) con gzip
3. **Portabilità**: Singolo file, facile distribuzione
4. **Memoria**: Gestione efficiente, no overflow

### ⚠️ Problemi Critici Identificati

#### 1. **Cone Search MOLTO Lente** (15-50 secondi!)
**Problema**: Scansione lineare di 303M record per ogni query
- Cone 0.5°: 15 secondi
- Cone 5°: 48 secondi  
- Count 3°: 49 secondi

**Causa**: Nessuna indicizzazione spaziale
```cpp
// Attualmente in gaia_mag18_catalog.cpp
for (uint64_t i = 0; i < header_.total_stars; ++i) {  // ❌ O(N) - LENTISSIMO!
    auto record = readRecord(i);
    double dist = angularDistance(ra, dec, record->ra, record->dec);
    if (dist <= radius) {
        results.push_back(recordToStar(*record));
    }
}
```

#### 2. **Decompressione Inefficiente**
**Problema**: gzseek() per ogni record = molto lento
```cpp
std::optional<Mag18Record> readRecord(uint64_t index) const {
    z_off_t pos = header_.data_offset + (index * sizeof(Mag18Record));
    gzseek(gz_file_, pos, SEEK_SET);  // ❌ Seek su file compresso = LENTO
    gzread(gz_file_, &record, sizeof(record));
}
```

#### 3. **Mancanza Proper Motion e Altri Parametri**
**Limitazione**: Record 52 bytes manca:
- Proper motion (PM RA, PM Dec) - critici per astrometria
- Errori astrometrici completi
- RUWE (quality indicator)
- Radial velocity
- Epoch propagation impossibile

#### 4. **Thread Safety Parziale**
**Problema**: gz_file_ condiviso, mutex solo su stats

---

## 🚀 MIGLIORAMENTI PROPOSTI

### Priorità 1: INDICIZZAZIONE SPAZIALE HEALPix

#### Implementazione
Aggiungere indice HEALPix NSIDE=64 al file:

```
[Header: 64 bytes]
[HEALPix Index: variable size]
  - NSIDE: 4 bytes
  - N_pixels: 4 bytes
  - Pixel offsets array: N_pixels × 8 bytes
[Sorted Records by HEALPix pixel]
```

#### Vantaggi
- ✅ Cone search da O(N) a O(pixels × stars_per_pixel)
- ✅ Regioni piccole: da 15s a **<100ms** (150x più veloce!)
- ✅ Backward compatible (versione header 2)

#### Codice
```cpp
struct Mag18CatalogHeaderV2 {
    char magic[8];
    uint32_t version;              // 2
    uint64_t total_stars;
    double mag_limit;
    uint64_t healpix_index_offset; // NEW
    uint32_t healpix_nside;        // NEW (es. 64)
    uint32_t healpix_npixels;      // NEW (12×64²=49152)
    uint64_t data_offset;
    uint64_t data_size;
    uint8_t reserved[12];
};

// Index array: pixel_id → (start_offset, count)
struct HEALPixIndexEntry {
    uint64_t start_offset;  // Offset primo record in pixel
    uint32_t star_count;    // Numero stelle nel pixel
    uint32_t reserved;
};
```

#### Query ottimizzata
```cpp
std::vector<GaiaStar> queryCone(double ra, double dec, double radius) {
    // 1. Trova pixel HEALPix interessati (10-100 pixel per raggio tipico)
    auto pixels = getPixelsInCone(ra, dec, radius);
    
    // 2. Leggi solo stelle in quei pixel
    for (auto pixel_id : pixels) {
        auto entry = healpix_index_[pixel_id];
        // Seek a inizio pixel e leggi solo quelle stelle
        gzseek(gz_file_, entry.start_offset, SEEK_SET);
        for (uint32_t i = 0; i < entry.star_count; ++i) {
            // Leggi sequenzialmente = VELOCE su gzip
        }
    }
}
```

**Stima miglioramento**:
- Cone 0.5°: da 15s a **50ms** (300x più veloce)
- Cone 5°: da 48s a **500ms** (96x più veloce)

---

### Priorità 2: DECOMPRESSIONE OTTIMIZZATA

#### Problema Attuale
gzip + seek random = disastroso per performance

#### Soluzione A: Formato Ibrido (Non compresso + Index compresso)
```
[Header]
[HEALPix Index - UNCOMPRESSED]  // Piccolo (~1 MB)
[Star Records - GZIP Compressed] // Grosso (9 GB)
```

Vantaggi:
- Index in RAM: accesso O(1)
- Records compressi: risparmio spazio
- Lettura sequenziale per pixel

#### Soluzione B: Chunk Compression (MIGLIORE!)
```
[Header]
[Chunk Index: N chunks × (offset, compressed_size, uncompressed_size)]
[Chunk 0: gzip(1M records)] ← 52 MB ciascuno
[Chunk 1: gzip(1M records)]
...
```

Query flow:
```cpp
// 1. Trova chunk che contengono la regione
auto chunks = getChunksForRegion(healpix_pixels);

// 2. Decomprimi interi chunk in memoria (veloce)
for (auto chunk_id : chunks) {
    auto data = decompressChunk(chunk_id);  // ~50ms per 52MB
    
    // 3. Cerca in memoria (velocissimo)
    for (auto& record : data) {
        if (inRegion(record)) results.push_back(record);
    }
}
```

**Vantaggi**:
- ✅ Decompressione sequenziale (10x più veloce)
- ✅ Cache di chunk depressi
- ✅ Parallelizzazione possibile

---

### Priorità 3: FORMATO RECORD ESTESO

#### Record V2: 80 bytes (compromesso)
```cpp
#pragma pack(push, 1)
struct Mag18RecordV2 {
    // Identificazione (8 bytes)
    uint64_t source_id;
    
    // Astrometria (32 bytes)
    double ra;           // 8 bytes
    double dec;          // 8 bytes
    float parallax;      // 4 bytes
    float parallax_error;// 4 bytes
    float pmra;          // 4 bytes - NEW!
    float pmdec;         // 4 bytes - NEW!
    
    // Fotometria (24 bytes)
    float g_mag;         // 4 bytes (era double)
    float bp_mag;        // 4 bytes
    float rp_mag;        // 4 bytes
    float g_mag_error;   // 4 bytes - NEW!
    float bp_mag_error;  // 4 bytes - NEW!
    float rp_mag_error;  // 4 bytes - NEW!
    
    // Quality (4 bytes)
    float ruwe;          // 4 bytes - NEW!
    
    // HEALPix (12 bytes)
    uint32_t healpix_pixel;  // 4 bytes - NEW!
    uint8_t reserved[8];     // 8 bytes
};
#pragma pack(pop)
```

**Vantaggi**:
- ✅ Proper motion → Epoch propagation
- ✅ Errori fotometrici → Analisi qualità
- ✅ RUWE → Filtro affidabilità
- ✅ HEALPix ID → Index diretto
- ✅ Solo +28 bytes (54% più grande, ma vale la pena)

**Trade-off**: 
- File: 9 GB → 14 GB compressed (~24 GB uncompressed)
- Valore: Molto maggiore per uso scientifico

---

### Priorità 4: CACHING INTELLIGENTE

#### Multi-Level Cache
```cpp
class Mag18CatalogOptimized {
private:
    // Level 1: LRU cache record individuali (10K record)
    std::map<uint64_t, Mag18Record> record_cache_;
    
    // Level 2: Cache chunk decompressi (5 chunk = 5M record)
    std::map<uint32_t, std::vector<Mag18Record>> chunk_cache_;
    
    // Level 3: Cache risultati query recenti
    std::map<QueryKey, std::vector<GaiaStar>> query_cache_;
    
    // Statistics
    CacheStats stats_;
};
```

#### Query Cache
```cpp
struct QueryKey {
    double ra;
    double dec;
    double radius;
    // Hash per map
    bool operator<(const QueryKey& o) const {
        return std::tie(ra, dec, radius) < std::tie(o.ra, o.dec, o.radius);
    }
};
```

**Vantaggi**:
- ✅ Query ripetute: da 15s a <1ms
- ✅ Regioni vicine: Riuso chunk
- ✅ Memory management automatico

---

### Priorità 5: PARALLELIZZAZIONE

#### Thread Pool per Query
```cpp
std::vector<GaiaStar> queryConeParallel(double ra, double dec, double radius) {
    auto pixels = getPixelsInCone(ra, dec, radius);
    
    // Processa pixel in parallelo
    std::vector<std::future<std::vector<GaiaStar>>> futures;
    
    for (auto pixel_id : pixels) {
        futures.push_back(thread_pool_.enqueue([=]() {
            return queryPixel(pixel_id, ra, dec, radius);
        }));
    }
    
    // Merge risultati
    std::vector<GaiaStar> results;
    for (auto& f : futures) {
        auto partial = f.get();
        results.insert(results.end(), partial.begin(), partial.end());
    }
    
    return results;
}
```

**Vantaggi**:
- ✅ Cone grandi: Speedup 4-8x su CPU multi-core
- ✅ I/O parallelo su SSD

---

### Priorità 6: QUERY AVANZATE

#### Spatial Join
```cpp
// Trova stelle vicine a lista di posizioni
std::vector<std::pair<size_t, GaiaStar>> queryCrossMatch(
    const std::vector<Coords>& positions,
    double radius
);
```

#### Range Queries Multi-Parametro
```cpp
struct QueryFilter {
    std::optional<double> ra_min, ra_max;
    std::optional<double> dec_min, dec_max;
    std::optional<double> mag_min, mag_max;
    std::optional<double> parallax_min, parallax_max;
    std::optional<double> pmra_min, pmra_max;
    std::optional<double> ruwe_max;  // Quality cut
};

std::vector<GaiaStar> queryFiltered(const QueryFilter& filter);
```

---

## 📊 STIMA IMPATTO MIGLIORAMENTI

### Versione Attuale (V1)
```
Format: 52 bytes/record, gzip, no index
Size: 9 GB
Query source_id: <1 ms ✅
Cone 0.5°: 15,000 ms ❌
Cone 5°: 48,000 ms ❌
Memory: 50 MB
```

### Versione Ottimizzata (V2 - Con tutti i miglioramenti)
```
Format: 80 bytes/record, chunk gzip, HEALPix NSIDE=64
Size: 14 GB (+55%)
Query source_id: <1 ms ✅
Cone 0.5°: 50 ms ✅ (300x più veloce!)
Cone 5°: 500 ms ✅ (96x più veloce!)
Cone 0.5° cached: <1 ms ✅
Parallel cone 10°: 200 ms ✅
Memory: 200 MB (cache chunks)
```

### Trade-off
| Aspetto | V1 | V2 | Differenza |
|---------|----|----|------------|
| **File size** | 9 GB | 14 GB | +55% |
| **Parametri** | 7 | 13 | +6 campi |
| **Cone small** | 15s | 50ms | **300x più veloce** |
| **Cone large** | 48s | 500ms | **96x più veloce** |
| **RAM usage** | 50 MB | 200 MB | +150 MB |
| **Build time** | 60 min | 90 min | +30 min |

**Conclusione**: +55% spazio per 100-300x performance = **VALE LA PENA!**

---

## 🎯 ROADMAP IMPLEMENTAZIONE

### Fase 1 (Urgente): HEALPix Index
**Tempo**: 2-3 giorni
**Impatto**: 100x velocità cone search
**Files**:
- `build_mag18_catalog_v2.cpp` - Genera con index
- `gaia_mag18_catalog_v2.h/cpp` - Legge con index
- Backward compatible (legge V1 e V2)

### Fase 2 (Alta priorità): Chunk Compression
**Tempo**: 2-3 giorni
**Impatto**: 2-5x ulteriore speedup
**Files**:
- Modifica formato chunk-based
- Cache management

### Fase 3 (Importante): Record Esteso
**Tempo**: 1-2 giorni
**Impatto**: Proper motion disponibile
**Files**:
- Nuovo formato 80 bytes
- Rigenerazione catalogo

### Fase 4 (Enhancement): Parallelizzazione
**Tempo**: 1-2 giorni
**Impatto**: 4-8x su multi-core
**Files**:
- Thread pool
- Query parallele

### Fase 5 (Nice to have): Query Avanzate
**Tempo**: 3-4 giorni
**Impatto**: Funzionalità aggiuntive
**Files**:
- Cross-match
- Filtri complessi

---

## 💡 ALTERNATIVE

### Opzione 1: SQLite Backend
```
Pros:
✅ Index automatici
✅ Query SQL potenti
✅ ACID transactions
✅ Mature, testato

Cons:
❌ File più grande (20-25 GB)
❌ Overhead SQL
❌ Meno controllo fine
```

### Opzione 2: Apache Arrow + Parquet
```
Pros:
✅ Columnar storage
✅ Compressione eccellente
✅ Zero-copy reads
✅ Standard industry

Cons:
❌ Dipendenza pesante
❌ Learning curve
❌ Overkill per caso d'uso?
```

### Opzione 3: Custom Binary + mmap
```
Pros:
✅ Performance massime
✅ Zero overhead
✅ Pieno controllo

Cons:
❌ Più complesso
❌ No compressione nativa
❌ File molto grande (24 GB uncompressed)
```

**Raccomandazione**: Restare su formato custom ottimizzato (V2)
- Controllo totale
- Compressione gzip (universale)
- Overhead minimo
- Nessuna dipendenza esterna

---

## 🔧 QUICK WINS (Implementabili Subito)

### 1. Pre-decomprimi Tutto
Se hai spazio su SSD:
```bash
# Decomprimi una volta
gunzip -k gaia_mag18.cat.gz
# Usa versione non compressa
./load_uncompressed gaia_mag18.cat
```
**Guadagno**: 2-3x velocità, ma +15 GB spazio

### 2. Memory-Map File Non Compresso
```cpp
// mmap = velocità memoria
int fd = open("gaia_mag18.cat", O_RDONLY);
void* data = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
```
**Guadagno**: Read istantanei, kernel gestisce cache

### 3. Batch Queries
Invece di:
```cpp
for (auto sid : source_ids) {
    auto star = catalog.queryStar(sid);  // 1000 seek
}
```
Fai:
```cpp
auto stars = catalog.queryStars(source_ids);  // 1 scan ordinato
```

---

## 📈 BENCHMARK OBIETTIVI

### Target Performance V2
```
Query Type              | Current  | Target   | Speedup
------------------------|----------|----------|--------
source_id (single)      | <1 ms    | <1 ms    | 1x ✅
source_id (batch 1000)  | 1000 ms  | 50 ms    | 20x
Cone 0.5° (no cache)    | 15,000ms | 50 ms    | 300x
Cone 0.5° (cached)      | 15,000ms | <1 ms    | 15000x
Cone 5° (no cache)      | 48,000ms | 500 ms   | 96x
Cone 10° (parallel)     | 120,000ms| 200 ms   | 600x
Count 3°                | 49,000ms | 300 ms   | 163x
queryBrightest 5° (top10)| 48,000ms| 600 ms   | 80x
```

---

## CONCLUSIONE

### Mag18 V1 (Attuale)
- ✅ Eccellente per: Query source_id, storage compatto
- ❌ Pessimo per: Cone search, query spaziali

### Mag18 V2 (Proposto)
- ✅ Eccellente per: TUTTO
- Trade-off: +5 GB (+55%) per 100-300x velocità

### Raccomandazione Finale
**IMPLEMENTARE V2 con Priorità 1+2+3**:
1. HEALPix index (100x speedup)
2. Chunk compression (5x speedup)
3. Record esteso 80 bytes (proper motion)

**Risultato**: Catalogo competitivo con GRAPPA3E per cone search, ma:
- ✅ Più compatto (14 GB vs 146 GB)
- ✅ Portabile (1 file vs 61,202)
- ✅ Source_id più veloce (binary search vs tile lookup)
- ✅ Sufficiente per 95% use cases (mag ≤ 18)

**GRAPPA3E resta necessario solo per**:
- Stelle deboli (G > 18)
- Query box (non circolari)
- Accesso casuali multipli (tile structure)
