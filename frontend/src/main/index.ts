import { app, BrowserWindow, ipcMain, dialog } from 'electron'
import { join, dirname } from 'path'
import { existsSync, mkdirSync, copyFileSync, appendFileSync } from 'fs'

let mainWindow: BrowserWindow | null = null
let windowEverShown = false
let errorsDialogShown = false

const startupErrors: string[] = []

let native: {
  initialize: (path: string) => boolean
  start: () => boolean
  stop: () => void
  getState: () => string
  loadConfig: (name: string) => boolean
  saveConfig: (name: string) => boolean
  getConfigJson: () => string
  setConfigJson: (json: string) => boolean
  validateConfig: (json: string) => string
  joystickConnected: () => boolean
  getCaptureMode: () => string
  keyboardAccessibility: () => boolean
  getActiveProfile: () => string
  listProfiles: () => string
  switchProfile: (name: string) => boolean
  createProfile: (name: string, copyCurrent: boolean) => boolean
  deleteProfile: (name: string) => boolean
} | null = null

let nativeLoadError: string | null = null

/** Log path yang bisa dipakai sebelum app.ready (penting di Windows). */
function earlyLogDir(): string {
  const base =
    process.env.APPDATA ||
    process.env.LOCALAPPDATA ||
    process.env.HOME ||
    process.cwd()
  return join(base, 'inputflow')
}

function earlyLogPath(): string {
  return join(earlyLogDir(), 'inputflow-startup.log')
}

function startupLog(message: string): void {
  const line = `${new Date().toISOString()} ${message}\n`
  console.error('[InputFlow]', message)
  try {
    const dir = earlyLogDir()
    if (!existsSync(dir)) mkdirSync(dir, { recursive: true })
    appendFileSync(earlyLogPath(), line, 'utf8')
  } catch {
    /* ignore */
  }
  try {
    if (app.isReady()) {
      const dir = join(app.getPath('userData'))
      if (!existsSync(dir)) mkdirSync(dir, { recursive: true })
      appendFileSync(join(dir, 'inputflow-startup.log'), line, 'utf8')
    }
  } catch {
    /* ignore */
  }
}

function pushError(message: string): void {
  startupErrors.push(message)
  startupLog(`ERROR: ${message}`)
}

/** Satu dialog berisi semua error — tanpa membuka jendela app. */
function showAllErrorsAndExit(title = 'InputFlow — tidak dapat dijalankan'): void {
  if (errorsDialogShown) return
  errorsDialogShown = true

  const list =
    startupErrors.length > 0
      ? startupErrors.map((e, i) => `${i + 1}. ${e}`).join('\n\n')
      : 'Terjadi kesalahan tidak diketahih (proses berhenti sebelum UI tampil).'

  const body = [
    list,
    '',
    `Log lengkap: ${earlyLogPath()}`,
    process.platform === 'win32'
      ? 'Jika modul native gagal: pasang VC++ Redistributable x64 (aka.ms/vs/17/release/vc_redist.x64.exe)'
      : ''
  ]
    .filter(Boolean)
    .join('\n')

  startupLog(`Showing error dialog (${startupErrors.length} errors)`)

  try {
    // showErrorBox sinkron di Windows — tidak butuh BrowserWindow
    dialog.showErrorBox(title, body)
  } catch (e) {
    startupLog(`showErrorBox failed: ${e}`)
  }

  app.exit(1)
}

function errMsg(e: unknown): string {
  if (e instanceof Error) return e.stack || e.message
  return String(e)
}

function nativeModuleCandidates(): string[] {
  if (app.isPackaged) {
    return [
      join(process.resourcesPath, 'native', 'inputflow_native.node'),
      join(process.resourcesPath, 'app.asar.unpacked', 'native', 'inputflow_native.node'),
      join(dirname(process.execPath), 'resources', 'native', 'inputflow_native.node')
    ]
  }
  return [
    join(process.cwd(), 'native/build/Release/inputflow_native.node'),
    join(__dirname, '../../native/build/Release/inputflow_native.node'),
    join(__dirname, '../../../native/build/Release/inputflow_native.node'),
    join(__dirname, '../../../../native/build/Release/inputflow_native.node')
  ]
}

function findNativeModulePath(): string | null {
  for (const p of nativeModuleCandidates()) {
    if (existsSync(p)) return p
  }
  return null
}

function pathsForPreflight(): { preloadPath: string; rendererHtml: string } {
  return {
    preloadPath: join(__dirname, '../preload/index.js'),
    rendererHtml: join(__dirname, '../renderer/index.html')
  }
}

/**
 * Cek semua prasyarat + coba muat native.
 * Jika ada error → tampilkan dialog gabungan, exit(1), tanpa jendela.
 */
function runStartupPreflight(): boolean {
  startupLog(
    `Preflight (packaged=${app.isPackaged}, platform=${process.platform}, exec=${process.execPath})`
  )

  const { preloadPath, rendererHtml } = pathsForPreflight()

  if (!existsSync(preloadPath)) {
    pushError(`Preload hilang: ${preloadPath}`)
  }
  if (!existsSync(rendererHtml)) {
    pushError(`UI (index.html) hilang: ${rendererHtml}`)
  }

  const configBundled = app.isPackaged
    ? join(process.resourcesPath, 'configs', 'default.json')
    : join(process.cwd(), 'configs', 'default.json')
  if (app.isPackaged && !existsSync(configBundled)) {
    pushError(`Config default hilang: ${configBundled}`)
  }

  const nativePath = findNativeModulePath()
  if (!nativePath) {
    pushError(`Modul native tidak ditemukan. Dicoba:\n${nativeModuleCandidates().join('\n')}`)
  } else {
    startupLog(`Native path: ${nativePath}`)
    try {
      // eslint-disable-next-line @typescript-eslint/no-require-imports
      native = require(nativePath)
      nativeLoadError = null
      startupLog('Native module loaded OK in preflight')
    } catch (e) {
      native = null
      nativeLoadError = errMsg(e)
      pushError(`Gagal memuat modul native:\n${nativePath}\n\n${nativeLoadError}`)
    }
  }

  if (startupErrors.length > 0) {
    showAllErrorsAndExit()
    return false
  }
  return true
}

function ensureNativeLoaded(): boolean {
  if (native) return true
  const path = findNativeModulePath()
  if (!path) {
    nativeLoadError = 'Modul native tidak ditemukan'
    pushError(nativeLoadError)
    return false
  }
  try {
    // eslint-disable-next-line @typescript-eslint/no-require-imports
    native = require(path)
    nativeLoadError = null
    return true
  } catch (e) {
    nativeLoadError = errMsg(e)
    pushError(`Gagal memuat native: ${nativeLoadError}`)
    return false
  }
}

function bundledConfigPath(): string {
  if (app.isPackaged) {
    return join(process.resourcesPath, 'configs', 'default.json')
  }
  return join(process.cwd(), 'configs', 'default.json')
}

function configDir(): string {
  const dir = join(app.getPath('userData'), 'configs')
  if (!existsSync(dir)) mkdirSync(dir, { recursive: true })
  const src = bundledConfigPath()
  const dst = join(dir, 'default.json')
  if (existsSync(src) && !existsSync(dst)) {
    try {
      copyFileSync(src, dst)
    } catch (e) {
      pushError(`Gagal menyalin config default: ${errMsg(e)}`)
    }
  }
  return dir
}

function createWindow(): void {
  const { preloadPath, rendererHtml } = pathsForPreflight()

  mainWindow = new BrowserWindow({
    width: 1440,
    height: 900,
    minWidth: 1200,
    minHeight: 760,
    backgroundColor: '#0B0F17',
    title: 'InputFlow',
    show: false,
    webPreferences: {
      preload: preloadPath,
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false
    }
  })

  mainWindow.once('ready-to-show', () => {
    windowEverShown = true
    mainWindow?.show()
    startupLog('Window ready-to-show')
  })

  mainWindow.on('closed', () => {
    mainWindow = null
  })

  mainWindow.webContents.on('did-fail-load', (_event, code, desc, url) => {
    pushError(`Gagal memuat UI (${code}): ${desc}\nURL: ${url}`)
    mainWindow?.destroy()
    mainWindow = null
    showAllErrorsAndExit('InputFlow — gagal memuat UI')
  })

  mainWindow.webContents.on('render-process-gone', (_event, details) => {
    pushError(`Renderer crash: ${details.reason} (exit ${details.exitCode})`)
    mainWindow?.destroy()
    mainWindow = null
    showAllErrorsAndExit('InputFlow — crash')
  })

  if (process.env.ELECTRON_RENDERER_URL) {
    mainWindow.loadURL(process.env.ELECTRON_RENDERER_URL).catch((e) => {
      pushError(`loadURL gagal: ${errMsg(e)}`)
      showAllErrorsAndExit()
    })
  } else {
    mainWindow.loadFile(rendererHtml).catch((e) => {
      pushError(`loadFile gagal: ${errMsg(e)}`)
      showAllErrorsAndExit()
    })
  }
}

function registerIpc(): void {
  ipcMain.handle('app:getBootstrap', () => ({
    platform: process.platform,
    nativeLoaded: native !== null,
    nativeModuleFound: findNativeModulePath() !== null,
    nativeLoadError,
    isPackaged: app.isPackaged
  }))

  ipcMain.handle('app:reportError', (_e, message: string) => {
    pushError(message)
    showAllErrorsAndExit('InputFlow — error')
  })

  ipcMain.handle('app:initialize', () => {
    try {
      if (!ensureNativeLoaded()) {
        showAllErrorsAndExit()
        return false
      }
      if (!native!.initialize(configDir())) {
        pushError('initialize() mengembalikan false')
        showAllErrorsAndExit()
        return false
      }
      return true
    } catch (e) {
      pushError(`initialize() exception: ${errMsg(e)}`)
      showAllErrorsAndExit()
      return false
    }
  })

  ipcMain.handle('app:start', () => {
    try {
      if (!ensureNativeLoaded()) {
        showAllErrorsAndExit()
        return false
      }
      return native!.start()
    } catch (e) {
      pushError(`start() exception: ${errMsg(e)}`)
      showAllErrorsAndExit()
      return false
    }
  })

  ipcMain.handle('app:stop', () => {
    try {
      native?.stop()
    } catch (e) {
      pushError(`stop() exception: ${errMsg(e)}`)
    }
  })

  ipcMain.handle('app:state', () => (native ? native.getState() : 'stopped'))
  ipcMain.handle('app:loadConfig', (_e, name: string) =>
    native ? native.loadConfig(name) : false
  )
  ipcMain.handle('app:saveConfig', (_e, name: string) =>
    native ? native.saveConfig(name) : false
  )
  ipcMain.handle('app:getConfigJson', () => (native ? native.getConfigJson() : '{}'))
  ipcMain.handle('app:setConfigJson', (_e, json: string) =>
    native ? native.setConfigJson(json) : false
  )
  ipcMain.handle('app:validateConfig', (_e, json: string) =>
    native ? native.validateConfig(json) : '[]'
  )
  ipcMain.handle('app:joystickConnected', () =>
    native ? native.joystickConnected() : false
  )
  ipcMain.handle('app:getCaptureMode', () => (native ? native.getCaptureMode() : 'shared'))
  ipcMain.handle('app:keyboardAccessibility', () => {
    if (!native) return false
    if (process.platform === 'win32') return true
    return native.keyboardAccessibility()
  })
  ipcMain.handle('app:getActiveProfile', () => (native ? native.getActiveProfile() : 'default'))
  ipcMain.handle('app:listProfiles', () => (native ? native.listProfiles() : '[]'))
  ipcMain.handle('app:switchProfile', (_e, name: string) =>
    native ? native.switchProfile(name) : false
  )
  ipcMain.handle('app:createProfile', (_e, name: string, copyCurrent: boolean) =>
    native ? native.createProfile(name, copyCurrent) : false
  )
  ipcMain.handle('app:deleteProfile', (_e, name: string) =>
    native ? native.deleteProfile(name) : false
  )
}

if (process.platform === 'win32') {
  app.disableHardwareAcceleration()
}

process.on('uncaughtException', (err) => {
  pushError(`uncaughtException: ${errMsg(err)}`)
  showAllErrorsAndExit('InputFlow — uncaughtException')
})

process.on('unhandledRejection', (reason) => {
  pushError(`unhandledRejection: ${errMsg(reason)}`)
  if (!windowEverShown) showAllErrorsAndExit('InputFlow — unhandledRejection')
})

app.whenReady().then(() => {
  try {
    if (!runStartupPreflight()) return

    registerIpc()
    createWindow()
  } catch (e) {
    pushError(`whenReady: ${errMsg(e)}`)
    showAllErrorsAndExit()
  }
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0 && !errorsDialogShown) {
    try {
      if (runStartupPreflight()) createWindow()
    } catch (e) {
      pushError(`activate: ${errMsg(e)}`)
      showAllErrorsAndExit()
    }
  }
})

function releaseNative(): void {
  if (!native) return
  try {
    native.stop()
    native.saveConfig(native.getActiveProfile())
  } catch (e) {
    startupLog(`releaseNative: ${errMsg(e)}`)
  }
}

app.on('before-quit', () => {
  releaseNative()
})

app.on('window-all-closed', () => {
  releaseNative()
  native = null

  if (process.platform === 'darwin') return

  if (!windowEverShown && !errorsDialogShown) {
    pushError(
      'Proses berhenti sebelum jendela UI tampil (kemungkinan crash GPU/driver atau file rusak).'
    )
    showAllErrorsAndExit()
    return
  }

  app.quit()
})
