# Gaia Catalog Downloads and Resources

## 📥 Catalog Files

### Mag18 V2 Catalog (Recommended)

**Status**: 🔨 Build from GRAPPA3E (pre-built version coming soon)

**Specifications**:
- **Size**: 14 GB
- **Stars**: 302,796,443 (G ≤ 18)
- **Format**: Binary with HEALPix spatial index
- **Performance**: Cone search 50-500ms

**Build Instructions**:
```bash
# 1. Download GRAPPA3E catalog (146 GB)
cd ~/catalogs
mkdir -p GRAPPA3E
cd GRAPPA3E

# Download from IMCCE FTP
wget -r -np -nH --cut-dirs=3 \
  ftp://ftp.imcce.fr/pub/catalogs/GRAPPA3E/

# 2. Build Mag18 V2 catalog
cd /path/to/IOC_GaiaLib/build/examples
./build_mag18_catalog_v2 ~/catalogs/GRAPPA3E ~/catalogs/gaia_mag18_v2.cat

# Time: ~75 minutes
# Output: ~/catalogs/gaia_mag18_v2.cat (14 GB)
```

**Future**: Pre-built download link will be added here when available.

---

### Mag18 V1 Catalog (Legacy)

**Status**: 🔨 Build from GRAPPA3E

**Specifications**:
- **Size**: 9 GB
- **Stars**: 302,796,443 (G ≤ 18)
- **Format**: Binary gzip compressed
- **Performance**: Cone search 15-48s (slow)

**Build Instructions**:
```bash
cd /path/to/IOC_GaiaLib/build/examples
./build_mag18_catalog ~/catalogs/GRAPPA3E ~/catalogs/gaia_mag18.cat.gz

# Time: ~60 minutes
# Output: ~/catalogs/gaia_mag18.cat.gz (9 GB)
```

⚠️ **Note**: V1 is legacy. Use V2 for better performance.

---

### GRAPPA3E Full Catalog (Optional)

**Source**: IMCCE (Institut de Mécanique Céleste et de Calcul des Éphémérides)

**Specifications**:
- **Size**: 56 GB compressed → 146 GB extracted
- **Stars**: 1,811,709,771 (all Gaia DR3)
- **Files**: 61,202 tiles (1°×1° RA/Dec grid)
- **Format**: Binary optimized
- **Performance**: Cone search 1-100ms

**Download Options**:

#### Option 1: FTP Direct (Recommended)
```bash
cd ~/catalogs/GRAPPA3E

# Method 1: wget recursive
wget -r -np -nH --cut-dirs=3 \
  ftp://ftp.imcce.fr/pub/catalogs/GRAPPA3E/

# Method 2: Individual archives (11 files, ~5 GB each)
wget ftp://ftp.imcce.fr/pub/catalogs/GRAPPA3E/GRAPPA3E_005-025+Zone_Sud.7z
wget ftp://ftp.imcce.fr/pub/catalogs/GRAPPA3E/GRAPPA3E_026-033.7z
# ... (see full list below)

# Extract all
for f in *.7z; do 7z x "$f"; done
```

**FTP Server**: `ftp://ftp.imcce.fr/pub/catalogs/GRAPPA3E/`

**Archive Files**:
1. `GRAPPA3E_005-025+Zone_Sud.7z` (~5 GB)
2. `GRAPPA3E_026-033.7z` (~5 GB)
3. `GRAPPA3E_034-042.7z` (~5 GB)
4. `GRAPPA3E_043-051.7z` (~5 GB)
5. `GRAPPA3E_052-061.7z` (~5 GB)
6. `GRAPPA3E_062-071.7z` (~5 GB)
7. `GRAPPA3E_072-082.7z` (~5 GB)
8. `GRAPPA3E_083-093.7z` (~5 GB)
9. `GRAPPA3E_094-105.7z` (~5 GB)
10. `GRAPPA3E_106-174.7z` (~5 GB)
11. `GRAPPA3E_Complements.7z` (~1 GB)

**Total**: ~56 GB compressed

#### Option 2: Download Script
```bash
#!/bin/bash
# download_grappa3e.sh

DEST_DIR="${HOME}/catalogs/GRAPPA3E"
BASE_URL="ftp://ftp.imcce.fr/pub/catalogs/GRAPPA3E"

mkdir -p "$DEST_DIR"
cd "$DEST_DIR"

# Archive list
ARCHIVES=(
    "GRAPPA3E_005-025+Zone_Sud.7z"
    "GRAPPA3E_026-033.7z"
    "GRAPPA3E_034-042.7z"
    "GRAPPA3E_043-051.7z"
    "GRAPPA3E_052-061.7z"
    "GRAPPA3E_062-071.7z"
    "GRAPPA3E_072-082.7z"
    "GRAPPA3E_083-093.7z"
    "GRAPPA3E_094-105.7z"
    "GRAPPA3E_106-174.7z"
    "GRAPPA3E_Complements.7z"
)

# Download
for archive in "${ARCHIVES[@]}"; do
    echo "Downloading $archive..."
    wget -c "${BASE_URL}/${archive}"
done

# Extract
echo "Extracting archives..."
for archive in *.7z; do
    echo "Extracting $archive..."
    7z x "$archive"
done

echo "Download and extraction complete!"
echo "Total size: $(du -sh . | cut -f1)"
```

**Usage**:
```bash
chmod +x download_grappa3e.sh
./download_grappa3e.sh
```

#### Option 3: Browser Download
Visit: https://ftp.imcce.fr/pub/catalogs/GRAPPA3E/

Download files manually via web browser.

---

## 🌐 Online Gaia Archive

**ESA Gaia Archive** (no download needed):
- **URL**: https://gea.esac.esa.int/archive/
- **Access**: TAP/ADQL queries via web or API
- **Data**: Full Gaia DR3 (1.8B sources)
- **Rate Limit**: 10 queries/minute (configurable)

**Usage with IOC_GaiaLib**:
```cpp
#include "ioc_gaialib/gaia_client.h"

GaiaClient client(GaiaRelease::DR3);
auto stars = client.queryCone(ra, dec, radius);
```

---

## 📊 Catalog Comparison

| Catalog | Size | Stars | Performance | Use Case |
|---------|------|-------|-------------|----------|
| **Mag18 V2** | 14 GB | 303M | Cone: 50ms | **Recommended** (95% cases) |
| Mag18 V1 | 9 GB | 303M | Cone: 15s | Legacy only |
| GRAPPA3E | 146 GB | 1.8B | Cone: 1-100ms | G > 18 or complete |
| Online | 0 GB | 1.8B | Cone: 5-20s | Testing, no disk space |

---

## 🛠️ Requirements

### Disk Space
- **Mag18 V2**: 20 GB (temp + output)
- **Mag18 V1**: 15 GB (temp + output)
- **GRAPPA3E**: 250 GB (compressed + extracted + temp)

### Software
- **7zip**: For extracting GRAPPA3E archives
  ```bash
  # macOS
  brew install p7zip
  
  # Linux
  sudo apt-get install p7zip-full
  ```

- **wget**: For downloading
  ```bash
  # macOS
  brew install wget
  
  # Linux (usually pre-installed)
  sudo apt-get install wget
  ```

### Build Tools
- CMake 3.15+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- libcurl, libxml2, zlib (usually pre-installed)

---

## ⏱️ Estimated Times

### Download Times (100 Mbps connection)
- **GRAPPA3E compressed**: 1-2 hours (56 GB)
- **Mag18 V2 pre-built**: 2-3 minutes (14 GB) - when available

### Build Times
- **GRAPPA3E extraction**: 30-60 minutes
- **Mag18 V2 generation**: 75 minutes
- **Mag18 V1 generation**: 60 minutes

### Total Setup Time
- **Mag18 V2 from scratch**: ~3 hours (download + extract + build)
- **Mag18 V2 from pre-built**: ~5 minutes (when available)

---

## 🚀 Quick Start

### Scenario 1: I Want Fastest Setup (Future)
```bash
# When pre-built becomes available:
wget https://example.com/gaia_mag18_v2.cat  # 14 GB
# Ready to use immediately!
```

### Scenario 2: I Have GRAPPA3E Already
```bash
# Build V2 directly
cd IOC_GaiaLib/build/examples
./build_mag18_catalog_v2 ~/catalogs/GRAPPA3E ~/catalogs/gaia_mag18_v2.cat
# 75 minutes
```

### Scenario 3: Starting from Zero
```bash
# 1. Download GRAPPA3E (1-2 hours)
cd ~/catalogs/GRAPPA3E
wget -r -np -nH --cut-dirs=3 ftp://ftp.imcce.fr/pub/catalogs/GRAPPA3E/

# 2. Extract (30-60 minutes)
for f in *.7z; do 7z x "$f"; done

# 3. Build V2 (75 minutes)
cd /path/to/IOC_GaiaLib/build/examples
./build_mag18_catalog_v2 ~/catalogs/GRAPPA3E ~/catalogs/gaia_mag18_v2.cat

# Total: ~3 hours
```

### Scenario 4: Testing Only (No Disk Space)
```cpp
// Use online archive (no download)
#include "ioc_gaialib/gaia_client.h"
GaiaClient client(GaiaRelease::DR3);
auto stars = client.queryCone(ra, dec, radius);
```

---

## 📝 Verification

After download/build, verify integrity:

```bash
# Check file size
ls -lh ~/catalogs/gaia_mag18_v2.cat
# Should be ~14 GB

# Test loading
cd IOC_GaiaLib/build/examples
./test_mag18_catalog_v2 ~/catalogs/gaia_mag18_v2.cat

# Expected output:
# ✅ Loaded 302,796,443 stars
# ✅ HEALPix pixels: ~6000
# ✅ Chunks: 303
# ✅ Cone search 0.5°: ~50 ms
```

---

## 🔗 Official Resources

- **GRAPPA3E FTP**: ftp://ftp.imcce.fr/pub/catalogs/GRAPPA3E/
- **GRAPPA3E Info**: https://www.imcce.fr/
- **Gaia Archive**: https://gea.esac.esa.int/archive/
- **Gaia DR3 Docs**: https://www.cosmos.esa.int/web/gaia/dr3
- **IOC_GaiaLib**: https://github.com/manvalan/IOC_GaiaLib

---

## ❓ FAQ

**Q: Where can I download pre-built Mag18 V2?**  
A: Currently being prepared. Check GitHub releases page for updates.

**Q: Can I use V1 instead of V2?**  
A: V1 works but is 300x slower for cone searches. Only use if storage is critical.

**Q: Do I need GRAPPA3E if I have Mag18 V2?**  
A: No. V2 covers 95% use cases. GRAPPA3E only needed for G > 18 stars.

**Q: Can I resume interrupted downloads?**  
A: Yes, use `wget -c` to continue downloads.

**Q: How much RAM needed?**  
A: Build: 4-8 GB. Runtime: 2 GB (V2), <1 GB (V1).

**Q: Multi-platform?**  
A: Yes. Catalogs are binary but portable across platforms.

---

## 📧 Support

Issues or questions? Open an issue on GitHub:
https://github.com/manvalan/IOC_GaiaLib/issues

---

**Last Updated**: November 27, 2025  
**Version**: 2.0.0
