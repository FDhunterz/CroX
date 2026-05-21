# InputFlow — Joystick → Keyboard

## Unduh installer (publik)

Rilis terbaru otomatis dari CI — bisa dibagikan ke siapa saja:

**https://github.com/FDhunterz/CroX/releases/latest**

| Platform | File |
|----------|------|
| Windows 10/11 (64-bit) | `InputFlow-Setup-*.exe` |
| macOS Apple Silicon | `InputFlow-*.dmg` |

### Windows — jika app tidak jalan setelah install

1. Pasang [Visual C++ Redistributable x64](https://aka.ms/vs/17/release/vc_redist.x64.exe) lalu buka ulang InputFlow.
2. Controller **PlayStation** perlu mode **XInput** (DS4Windows, Steam Input, atau driver Xbox).
3. Buka **joy.cpl** (Win+R → `joy.cpl`) — pastikan gamepad terlihat.
4. Klik **Start** di app; status header harus berubah ke **Active** / `running`.

---

Aplikasi desktop (Windows & macOS) yang mengubah input **joystick menjadi tombol keyboard**, dengan pemetaan yang bisa ditambah/hapus manual oleh user.

## Fitur saat ini

- Pemetaan tombol joystick → keyboard (+ modifier Ctrl/Shift/Alt/Meta)
- **LB** default → Alt + Shift  
- **RB** default → Ctrl + Shift  
- Tambah / hapus pemetaan bebas
- Validasi: **tombol joystick tidak boleh duplikat**, **output keyboard tidak boleh sama**
- Start / Stop mapping engine
- Simpan / muat JSON

## Dihapus (sementara)

Macro, global keyboard hook, key remapping terpisah, delay editor, activity console multi-tab, dan fitur lama lainnya.

## Stack

| Layer | Tech |
|-------|------|
| Backend | C++20 |
| Joystick | XInput (Windows), GameController (macOS) |
| Keyboard output | SendInput / CGEventPost |
| UI | Electron + React + TypeScript |
| Bridge | N-API |

## Struktur

```
backend/     inputflow_core — joystick poll + mapping + keyboard
frontend/    UI pemetaan saja
native/      N-API bridge
configs/     default.json
shared/      types.hpp
```

## Config contoh (`configs/default.json`)

```json
{
  "mappings": [
    { "joystickButton": "LB", "key": "", "modifiers": ["Alt", "Shift"] },
    { "joystickButton": "RB", "key": "", "modifiers": ["Ctrl", "Shift"] }
  ]
}
```

## Build

```bash
cmake -B build && cmake --build build
cd native && npm install && npm run build   # optional N-API
cd frontend && npm install && npm run dev
```

**macOS:** izin **Accessibility** untuk output keyboard sintetis.

## API native (baru)

- `initialize`, `start`, `stop`, `getState`
- `getConfigJson`, `setConfigJson`, `validateConfig`
- `loadConfig`, `saveConfig`
- `joystickConnected`
