#!/bin/sh
# Assemble the web gallery: copy each saver's emscripten build into web/<name>/
# and drop in the gallery index. Run after `make browser`.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
mkdir -p web/shots
for d in savers/*/; do
  s=$(basename "$d")
  if [ -f "savers/$s/web/$s.html" ]; then
    mkdir -p "web/$s"
    cp savers/$s/web/$s.html savers/$s/web/$s.js savers/$s/web/$s.wasm "web/$s/" 2>/dev/null || true
    cp savers/$s/web/$s.data "web/$s/" 2>/dev/null || true
    # gallery thumbnail (docs/screenshots/<s>-linux-gl4es.png -> shots/<s>.png)
    cp "docs/screenshots/$s-linux-gl4es.png" "web/shots/$s.png" 2>/dev/null || true
    echo "deployed $s"
  fi
done
cp scripts/gallery.html web/index.html
echo "Gallery assembled in web/. Serve with: cd web && python3 -m http.server"
