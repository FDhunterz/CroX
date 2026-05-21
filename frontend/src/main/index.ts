import { app, BrowserWindow, ipcMain } from 'electron'
import { join } from 'path'
import { existsSync, mkdirSync, copyFileSync } from 'fs'

let mainWindow: BrowserWindow | null = null

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

function loadNativeModule(): void {
  const candidates: string[] = []

  if (app.isPackaged) {
    candidates.push(join(process.resourcesPath, 'native', 'inputflow_native.node'))
  } else {
    candidates.push(
      join(process.cwd(), 'native/build/Release/inputflow_native.node'),
      join(__dirname, '../../native/build/Release/inputflow_native.node'),
      join(__dirname, '../../../native/build/Release/inputflow_native.node'),
      join(__dirname, '../../../../native/build/Release/inputflow_native.node')
    )
  }

  for (const p of candidates) {
    if (existsSync(p)) {
      // eslint-disable-next-line @typescript-eslint/no-require-imports
      native = require(p)
      console.log('[InputFlow] Native loaded:', p)
      return
    }
  }
  console.warn('[InputFlow] Native module not found — UI-only mode')
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
  if (existsSync(src) && !existsSync(dst)) copyFileSync(src, dst)
  return dir
}

function createWindow(): void {
  mainWindow = new BrowserWindow({
    width: 1440,
    height: 900,
    minWidth: 1200,
    minHeight: 760,
    backgroundColor: '#0B0F17',
    title: 'InputFlow',
    webPreferences: {
      preload: join(__dirname, '../preload/index.js'),
      contextIsolation: true,
      nodeIntegration: false
    }
  })

  if (process.env.ELECTRON_RENDERER_URL) {
    mainWindow.loadURL(process.env.ELECTRON_RENDERER_URL)
  } else {
    mainWindow.loadFile(join(__dirname, '../renderer/index.html'))
  }
}

function registerIpc(): void {
  ipcMain.handle('app:initialize', () => native?.initialize(configDir()) ?? false)
  ipcMain.handle('app:start', () => native?.start() ?? false)
  ipcMain.handle('app:stop', () => {
    native?.stop()
  })
  ipcMain.handle('app:state', () => native?.getState() ?? 'stopped')
  ipcMain.handle('app:loadConfig', (_e, name: string) => native?.loadConfig(name) ?? false)
  ipcMain.handle('app:saveConfig', (_e, name: string) => native?.saveConfig(name) ?? false)
  ipcMain.handle('app:getConfigJson', () => native?.getConfigJson() ?? '{}')
  ipcMain.handle('app:setConfigJson', (_e, json: string) => native?.setConfigJson(json) ?? false)
  ipcMain.handle('app:validateConfig', (_e, json: string) => native?.validateConfig(json) ?? '[]')
  ipcMain.handle('app:joystickConnected', () => native?.joystickConnected() ?? false)
  ipcMain.handle('app:getCaptureMode', () => native?.getCaptureMode() ?? 'shared')
  ipcMain.handle('app:keyboardAccessibility', () => native?.keyboardAccessibility() ?? false)
  ipcMain.handle('app:getActiveProfile', () => native?.getActiveProfile() ?? 'default')
  ipcMain.handle('app:listProfiles', () => native?.listProfiles() ?? '[]')
  ipcMain.handle('app:switchProfile', (_e, name: string) => native?.switchProfile(name) ?? false)
  ipcMain.handle('app:createProfile', (_e, name: string, copyCurrent: boolean) =>
    native?.createProfile(name, copyCurrent) ?? false
  )
  ipcMain.handle('app:deleteProfile', (_e, name: string) => native?.deleteProfile(name) ?? false)
}

app.whenReady().then(() => {
  loadNativeModule()
  registerIpc()
  createWindow()
})

function releaseNative(): void {
  if (!native) return
  try {
    native.stop()
    native.saveConfig(native.getActiveProfile())
  } catch (e) {
    console.error('[InputFlow] releaseNative:', e)
  }
}

app.on('before-quit', () => {
  releaseNative()
})

app.on('window-all-closed', () => {
  releaseNative()
  native = null
  if (process.platform !== 'darwin') app.quit()
})
