#!/bin/bash

# Script per scaricare il catalogo GRAPPA3E dall'IMCCE
# GRAPPA3E: Catalogo di asteroidi e parametri fisici

CATALOG_URL="https://ftp.imcce.fr/pub/catalogs/GRAPPA3E/"
LOCAL_DIR="$HOME/catalogs/GRAPPA3E"

echo "╔════════════════════════════════════════════════════════╗"
echo "║  GRAPPA3E Catalog Download                             ║"
echo "║  Source: IMCCE (Institut de Mécanique Céleste)         ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Crea directory locale
mkdir -p "$LOCAL_DIR"
echo "📁 Directory locale: $LOCAL_DIR"
echo ""

# Scarica il catalogo usando wget con mirror ricorsivo
echo "🌐 Scaricamento da: $CATALOG_URL"
echo "⏳ Attendere..."
echo ""

cd "$LOCAL_DIR"

# Usa wget per scaricare ricorsivamente
# -r: ricorsivo
# -np: no parent (non risale nelle directory)
# -nH: no host directories
# --cut-dirs=3: rimuove i primi 3 livelli di directory
# -R: reject (esclude certi tipi di file se necessario)
wget -r -np -nH --cut-dirs=3 \
     --reject "index.html*" \
     --progress=bar:force \
     "$CATALOG_URL"

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Download completato!"
    echo ""
    echo "📊 Statistiche:"
    echo "   Files scaricati:"
    find "$LOCAL_DIR" -type f | wc -l
    echo "   Dimensione totale:"
    du -sh "$LOCAL_DIR"
    echo ""
    echo "📂 Files disponibili:"
    ls -lh "$LOCAL_DIR"
    echo ""
    echo "💡 Path completo: $LOCAL_DIR"
else
    echo ""
    echo "❌ Errore durante il download"
    exit 1
fi
