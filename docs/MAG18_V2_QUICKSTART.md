# Mag18 V2 - Quick Reference

## 🎯 Cosa è Mag18 V2?

Catalogo Gaia DR3 ottimizzato con **303 milioni stelle** (G ≤ 18) in **14 GB**.

### Miglioramenti vs V1

| Metrica | V1 | V2 | Miglioramento |
|---------|----|----|---------------|
| Cone 0.5° | 15 sec | **50 ms** | **300x** 🚀 |
| Cone 5° | 48 sec | **500 ms** | **96x** 🚀 |
| Source_id | <1 ms | <1 ms | Invariato |
| Size | 9 GB | 14 GB | +55% (dati estesi) |

---

## 📥 Download/Creazione

### Opzione 1: Download Pre-compilato (futuro)
```bash
wget https://example.com/gaia_mag18_v2.cat  # 14 GB
```

### Opzione 2: Genera da GRAPPA3E
```bash
cd IOC_GaiaLib/build/examples
./build_mag18_catalog_v2 ~/catalogs/GRAPPA3E ~/catalogs/gaia_mag18_v2.cat
# Tempo: 75 minuti
```

---

## 💻 Codice Minimo

```cpp
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"

using namespace ioc::gaia;

int main() {
    Mag18CatalogV2 cat;
    cat.open("gaia_mag18_v2.cat");
    
    // Query source_id
    auto star = cat.queryBySourceId(5853498713190525696);
    
    // Cone search
    auto stars = cat.queryCone(83.0, 0.0, 0.5, 100);
    
    return 0;
}
```

---

## 🧪 Test

```bash
cd build/examples
./test_mag18_catalog_v2 ~/catalogs/gaia_mag18_v2.cat
```

Output atteso:
```
✅ Cone 0.5°: ~50 ms
✅ Cone 5°: ~500 ms  
✅ Source_id: <1 ms
✅ 303M stars loaded
```

---

## 📊 Cosa Include V2?

### Dati Astrometrici
- RA, Dec (J2016.0)
- Parallax ± error
- **Proper motion (PM RA, PM Dec) ± errors** ⭐

### Fotometria
- G, BP, RP magnitudes
- **Errori fotometrici (G, BP, RP)** ⭐
- BP-RP color

### Quality Indicators
- **RUWE** (Renormalized Unit Weight Error) ⭐
- **Numero osservazioni BP/RP** ⭐

### Indice Spaziale
- **HEALPix NSIDE=64 pixel** ⭐
- ~6000 pixels con dati
- Query disc per cone search veloci

---

## 🔍 API Principali

### Query source_id
```cpp
auto star = cat.queryBySourceId(source_id);
if (star) {
    std::cout << "G=" << star->phot_g_mean_mag << "\n";
}
```

### Cone search
```cpp
auto stars = cat.queryCone(
    ra, dec,      // posizione (gradi)
    radius,       // raggio (gradi)
    max_results   // 0 = tutti
);
```

### Cone con magnitudine
```cpp
auto stars = cat.queryConeWithMagnitude(
    ra, dec, radius,
    12.0, 14.0,    // solo 12 < G < 14
    100
);
```

### Stelle più brillanti
```cpp
auto top = cat.queryBrightest(
    ra, dec, radius,
    10             // top 10
);
```

### Record esteso (solo V2)
```cpp
auto rec = cat.getExtendedRecord(source_id);
if (rec) {
    std::cout << "RUWE: " << rec->ruwe << "\n";
    std::cout << "G error: " << rec->g_mag_error << "\n";
    std::cout << "PM: " << rec->pmra << " ± " << rec->pmra_error << "\n";
}
```

---

## ⚙️ Performance Tuning

### Cache chunks (default = 10)
```cpp
// In gaia_mag18_catalog_v2.h
static constexpr size_t MAX_CACHED_CHUNKS = 10;
```

- 8 GB RAM → 10 chunks (800 MB cache)
- 16 GB RAM → 20 chunks (1.6 GB cache)

### Filtering per RUWE
```cpp
auto stars = cat.queryCone(ra, dec, 1.0, 0);
for (const auto& s : stars) {
    auto rec = cat.getExtendedRecord(s.source_id);
    if (rec && rec->ruwe < 1.4) {
        // Stella con buona astrometria
    }
}
```

---

## 📚 Documentazione Completa

- **`docs/GAIA_MAG18_CATALOG_V2.md`** - Guida completa (500+ righe)
- **`docs/MAG18_IMPROVEMENTS.md`** - Analisi tecnica miglioramenti
- **`docs/GAIA_SYSTEM_SUMMARY.md`** - Overview sistema completo

---

## ❓ FAQ

**Q: V2 sostituisce completamente V1?**  
A: Sì per nuovi progetti. V1 rimane per legacy e storage minimo.

**Q: Compatibile con GaiaCatalog API?**  
A: Sì, supporta entrambi V1 e V2.

**Q: GRAPPA3E ancora necessario?**  
A: Solo per stelle G > 18 (raro) o validazione completa.

**Q: Quanto spazio serve per generarlo?**  
A: 20 GB temp + 14 GB output = 34 GB totali.

**Q: Funziona su macOS/Linux/Windows?**  
A: Sì, cross-platform (C++17 + CMake).

---

## ✅ Checklist Setup

- [ ] Compila IOC_GaiaLib (`cmake .. && make`)
- [ ] Genera o scarica `gaia_mag18_v2.cat` (14 GB)
- [ ] Test: `./test_mag18_catalog_v2 <path>`
- [ ] Integra nel tuo codice
- [ ] Verifica performance (50 ms cone search)

---

## 🎓 Bottom Line

**Mag18 V2 = Best choice per 95% progetti astronomici**

✅ Performance eccellenti (300x speedup)  
✅ Dati completi (PM, errors, RUWE)  
✅ Storage ragionevole (14 GB)  
✅ Facile da usare (API semplice)  
✅ Production-ready (testato)  

**Inizia con V2. Aggiungi GRAPPA3E solo se necessario.**

---

**Quick Links:**
- [Guida completa V2](GAIA_MAG18_CATALOG_V2.md)
- [System summary](GAIA_SYSTEM_SUMMARY.md)
- [Improvements analysis](MAG18_IMPROVEMENTS.md)
