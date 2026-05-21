#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP_SRC="$ROOT/frontend/release/mac-arm64/InputFlow.app"
APP_DST="/Applications/InputFlow.app"

if [[ ! -d "$APP_SRC" ]]; then
  echo "InputFlow.app belum di-build. Jalankan dulu:"
  echo "  ./scripts/build-mac-app.sh"
  exit 1
fi

echo "Menyalin ke $APP_DST ..."
rm -rf "$APP_DST"
cp -R "$APP_SRC" "$APP_DST"
echo "Selesai. Buka dari Launchpad atau: open -a InputFlow"
