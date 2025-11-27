#!/bin/bash

# IOC_GaiaLib + GRAPPA3E Setup Script
# Configura l'ambiente completo per la libreria Gaia e il catalogo asteroidi

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CATALOG_DIR="$HOME/catalogs/GRAPPA3E"

echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  IOC_GaiaLib + GRAPPA3E Setup                             ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# Check dependencies
echo "🔍 Checking dependencies..."
MISSING_DEPS=""

if ! command -v cmake &> /dev/null; then
    echo "  ❌ cmake not found"
    MISSING_DEPS="$MISSING_DEPS cmake"
else
    echo "  ✅ cmake $(cmake --version | head -1)"
fi

if ! command -v curl &> /dev/null; then
    echo "  ❌ curl not found"
    MISSING_DEPS="$MISSING_DEPS curl"
else
    echo "  ✅ curl installed"
fi

if ! command -v 7z &> /dev/null; then
    echo "  ⚠️  7z (p7zip) not found - needed for GRAPPA3E extraction"
    echo "     Install with: brew install p7zip"
    MISSING_DEPS="$MISSING_DEPS p7zip"
else
    echo "  ✅ 7z installed"
fi

if [ -n "$MISSING_DEPS" ]; then
    echo ""
    echo "❌ Missing dependencies:$MISSING_DEPS"
    echo "   Install with: brew install$MISSING_DEPS"
    exit 1
fi

echo ""
echo "✅ All dependencies satisfied"
echo ""

# Build IOC_GaiaLib
echo "🔨 Building IOC_GaiaLib..."
cd "$PROJECT_ROOT"
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)

if [ $? -eq 0 ]; then
    echo "✅ IOC_GaiaLib built successfully"
else
    echo "❌ Build failed"
    exit 1
fi

echo ""

# Setup GRAPPA3E
echo "📂 Setting up GRAPPA3E catalog..."
mkdir -p "$CATALOG_DIR"

# Check if documentation is already downloaded
if [ ! -f "$CATALOG_DIR/GRAPPA3E_En_V3.0.pdf" ]; then
    echo "  📥 Downloading documentation..."
    cd "$CATALOG_DIR"
    curl -O https://ftp.imcce.fr/pub/catalogs/GRAPPA3E/GRAPPA3E_En_V3.0.pdf
    curl -O https://ftp.imcce.fr/pub/catalogs/GRAPPA3E/Exemple_v3.7z
    echo "  ✅ Documentation downloaded"
else
    echo "  ✅ Documentation already present"
fi

# Check download status
echo ""
echo "📊 GRAPPA3E Catalog Status:"
ARCHIVE_COUNT=$(ls "$CATALOG_DIR"/GRAPPA3E_*.7z 2>/dev/null | wc -l | tr -d ' ')
echo "  Archives downloaded: $ARCHIVE_COUNT / 12"

if [ "$ARCHIVE_COUNT" -lt 12 ]; then
    echo ""
    echo "⚠️  Full GRAPPA3E catalog not downloaded (~55 GB)"
    echo ""
    read -p "Download full catalog now? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "🌐 Starting download (this will take 1-2 hours)..."
        cd "$CATALOG_DIR"
        if [ -f "$SCRIPT_DIR/download_grappa3e.sh" ]; then
            bash "$SCRIPT_DIR/download_grappa3e.sh"
        else
            # Inline download script
            BASE_URL="https://ftp.imcce.fr/pub/catalogs/GRAPPA3E"
            FILES=(
                "GRAPPA3E_005-025+Zone%20Sud.7z"
                "GRAPPA3E_026-033.7z"
                "GRAPPA3E_034-044.7z"
                "GRAPPA3E_045-054.7z"
                "GRAPPA3E_055-062.7z"
                "GRAPPA3E_063-071.7z"
                "GRAPPA3E_072-085.7z"
                "GRAPPA3E_086-105.7z"
                "GRAPPA3E_106-127.7z"
                "GRAPPA3E_128-174+Zone%20Nord.7z"
                "GRAPPA3E_Complements.7z"
                "GRAPPAvar.7z"
            )
            
            for file in "${FILES[@]}"; do
                echo "  📥 $file"
                curl -C - -O "$BASE_URL/$file"
            done
        fi
    else
        echo "  ⏭️  Skipping download"
        echo "     You can download later with: cd ~/catalogs/GRAPPA3E && ../IOC_GaiaLib/scripts/download_grappa3e.sh"
    fi
fi

# Extract if needed
EXTRACTED_FILES=$(find "$CATALOG_DIR" -type f \( -name "*.dat" -o -name "*.bin" -o -name "*.txt" -o -name "*.csv" \) 2>/dev/null | wc -l | tr -d ' ')

if [ "$EXTRACTED_FILES" -eq 0 ] && [ "$ARCHIVE_COUNT" -gt 0 ]; then
    echo ""
    echo "📦 Archives need extraction"
    read -p "Extract archives now? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "  🗜️  Extracting archives (this may take 30-60 minutes)..."
        cd "$CATALOG_DIR"
        for archive in GRAPPA3E_*.7z; do
            if [ -f "$archive" ]; then
                echo "    Extracting $archive..."
                7z x -y "$archive" > /dev/null
            fi
        done
        echo "  ✅ Extraction complete"
    fi
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  Setup Complete!                                          ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""
echo "📁 Project: $PROJECT_ROOT"
echo "🔧 Build:   $PROJECT_ROOT/build"
echo "📂 GRAPPA3E: $CATALOG_DIR"
echo ""
echo "🚀 Quick Start:"
echo ""
echo "  # Run basic tests"
echo "  cd $PROJECT_ROOT/build/examples"
echo "  ./simple_test"
echo "  ./real_data_test"
echo ""
echo "  # Test GRAPPA3E integration"
echo "  ./test_grappa"
echo ""
echo "  # Compile local Gaia catalog"
echo "  ./compile_catalog"
echo ""
echo "📚 Documentation:"
echo "  $PROJECT_ROOT/README.md"
echo "  $PROJECT_ROOT/docs/GRAPPA3E_IMPLEMENTATION.md"
echo "  $CATALOG_DIR/GRAPPA3E_En_V3.0.pdf"
echo ""
