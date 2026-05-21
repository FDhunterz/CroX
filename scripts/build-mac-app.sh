#!/usr/bin/env bash
# Build InputFlow.app untuk macOS (arm64)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "==> [1/3] Build native module (Electron 28 / arm64)"
cd native
npm install
npm run build
cd "$ROOT"

NATIVE_NODE="native/build/Release/inputflow_native.node"
if [[ ! -f "$NATIVE_NODE" ]]; then
  echo "ERROR: Native module tidak ditemukan: $NATIVE_NODE"
  exit 1
fi

echo "==> [2/3] Build Electron frontend"
cd frontend
npm install
npm run build

echo "==> [3/3] Package InputFlow.app (electron-builder)"
npm run dist:mac
cd "$ROOT"

APP_PATH="$ROOT/frontend/release/mac-arm64/InputFlow.app"
if [[ -d "$APP_PATH" ]]; then
  echo ""
  echo "Berhasil: $APP_PATH"
  echo ""
  echo "Install ke Applications:"
  echo "  cp -R \"$APP_PATH\" /Applications/"
  echo ""
  echo "Atau buka DMG di: frontend/release/InputFlow-*-arm64.dmg"
else
  echo "ERROR: .app tidak ditemukan di release/"
  exit 1
fi
