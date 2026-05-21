# Cara install InputFlow

Unduh dari: https://github.com/FDhunterz/CroX/releases/latest

---

## macOS (Apple Silicon)

1. Unduh file `InputFlow-<versi>-arm64.dmg`
2. Buka DMG, tarik **InputFlow** ke **Applications**
3. Jika muncul **"is damaged and can't be opened"** — ini normal untuk app unsigned:

```bash
xattr -cr /Applications/InputFlow.app
open /Applications/InputFlow.app
```

4. Beri izin **Accessibility** saat diminta (System Settings → Privacy → Accessibility → centang InputFlow)
5. Colok controller → buka app → **Start**

---

## Windows 10/11 (64-bit)

1. Unduh `InputFlow-Setup-<versi>.exe`
2. Jalankan installer (Next → Install)
3. Pasang [Visual C++ Redistributable x64](https://aka.ms/vs/17/release/vc_redist.x64.exe) jika app tertutup atau muncul error modul native
4. Buka InputFlow dari Start Menu
5. Jika ada **dialog error**, catat pesannya; log ada di:

   `%APPDATA%\inputflow\inputflow-startup.log`

6. Controller PlayStation: gunakan **DS4Windows** atau **Steam Input** (mode Xbox / XInput)
7. Cek gamepad: Win+R → `joy.cpl`

---

## Masih bermasalah?

Buka issue di GitHub dengan: versi OS, screenshot error, dan isi file log di atas.
