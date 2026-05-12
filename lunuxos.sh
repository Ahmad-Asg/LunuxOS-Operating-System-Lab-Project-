#!/bin/bash
# ═══════════════════════════════════════════════════════════
#   LunuxOS Launcher Script
#   Usage: ./lunuxos.sh  (or copy to /usr/local/bin/lunuxos)
# ═══════════════════════════════════════════════════════════

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

echo ""
echo "  LunuxOS Launcher"
echo "  ──────────────────"

# Check for GCC
if ! command -v gcc &>/dev/null; then
    echo "  ERROR: gcc not found. Install with: sudo apt install gcc"
    exit 1
fi

# Check for pthread library
if ! ldconfig -p 2>/dev/null | grep -q libpthread; then
    echo "  WARNING: libpthread not found in ldconfig cache."
fi

# Build if binary not present or source is newer
if [ ! -f "./lunuxos" ] || [ "./main.c" -nt "./lunuxos" ]; then
    echo "  Building LunuxOS..."
    make -j"$(nproc)" 2>&1
    if [ $? -ne 0 ]; then
        echo ""
        echo "  BUILD FAILED. Check errors above."
        exit 1
    fi
fi

echo "  Starting LunuxOS..."
echo ""
exec ./lunuxos
