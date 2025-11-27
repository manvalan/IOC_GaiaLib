# Quick Start - Gaia Mag18 Catalog

## Creazione del Catalogo Compresso

### 1. Prerequisiti
```bash
# Verifica che GRAPPA3E sia estratto
ls ~/catalogs/GRAPPA3E/
# Dovresti vedere 170 directories numerati (5-174)
```

### 2. Compila i tool
```bash
cd /Users/michelebigi/VisualStudio\ Code/GitHub/IOC_GaiaLib/build
cmake ..
make -j4
```

### 3. Crea il catalogo Mag18
```bash
cd examples
./build_mag18_catalog ~/catalogs/GRAPPA3E gaia_mag18.cat.gz
```

**Output atteso**:
- Tempo: 30-60 minuti
- Dimensione: ~2-3 GB (da 146 GB originale)
- ~150 milioni di stelle con G ≤ 18

### 4. Testa il catalogo
```bash
./test_mag18_catalog gaia_mag18.cat.gz
```

## Uso in Codice

```cpp
#include "ioc_gaialib/gaia_mag18_catalog.h"

int main() {
    GaiaMag18Catalog catalog("gaia_mag18.cat.gz");
    
    // Query VELOCE per source_id (binary search)
    auto star = catalog.queryBySourceId(12345678901234);
    if (star) {
        std::cout << "RA: " << star->ra 
                  << " Dec: " << star->dec 
                  << " G: " << star->phot_g_mean_mag << "\n";
    }
    
    // Cone search (più lento per grandi regioni)
    auto stars = catalog.queryCone(180.0, 0.0, 1.0, 100);
    std::cout << "Trovate " << stars.size() << " stelle\n";
    
    // Stelle brillanti
    auto bright = catalog.queryBrightest(180.0, 0.0, 2.0, 10);
    
    return 0;
}
```

## Vantaggi

✅ **70% più piccolo**: 2-3 GB vs 146 GB  
✅ **Veloce per source_id**: <1 ms (binary search)  
✅ **Un singolo file**: Facile da distribuire  
✅ **Compresso**: Decompressione automatica  

## Limitazioni

⚠️ **Cone search lente**: Scansione lineare (OK per raggio < 5°)  
⚠️ **Solo mag ≤ 18**: Stelle più deboli escluse  
⚠️ **Parametri limitati**: Solo essenziali (RA, Dec, mags, parallax)  

## Quando Usare

**Usa Mag18 Catalog per**:
- Query frequenti per source_id noti
- Storage/bandwidth limitati
- Backup/archivio
- Distribuzione semplificata

**Usa GRAPPA3E Full per**:
- Cone search intensive
- Stelle deboli (G > 18)
- Tutti i parametri Gaia
- Performance critiche cone search

## Documentazione Completa

Vedi: `docs/GAIA_MAG18_CATALOG.md`
