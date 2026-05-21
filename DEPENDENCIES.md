# Recommended Dependencies

## Build-time

| Dependency | Version | Install |
|------------|---------|---------|
| CMake | ≥ 3.20 | brew / apt / visualstudio |
| C++ compiler | C++20 | clang / MSVC |
| Node.js | ≥ 18 LTS | nodejs.org |
| Python 3 | 3.x | node-gyp requirement |

## C++ (managed by CMake FetchContent)

- **nlohmann/json** v3.11.3 — configuration serialization

## Node / Electron

- **electron** ^28 — desktop shell
- **electron-vite** ^2 — bundler
- **node-addon-api** ^8 — N-API C++ helpers
- **cmake-js** ^7 — compile native code for Electron ABI

## Optional (future phases)

- **electron-builder** — installers (DMG, NSIS)
- **Google Test** — backend unit tests (`INPUTFLOW_BUILD_TESTS=ON`)
- **fmt** — structured logging
- **spdlog** — log sinks

## Platform SDKs (system)

### Windows
- Windows SDK
- `user32.lib` — hooks and `SendInput`

### macOS
- macOS SDK 11+
- Frameworks: ApplicationServices, Carbon, CoreFoundation
