import { contextBridge, ipcRenderer } from 'electron'

export interface BootstrapInfo {
  platform: string
  nativeLoaded: boolean
  nativeLoadError: string | null
  isPackaged: boolean
}

export interface InputFlowApi {
  getBootstrap: () => Promise<BootstrapInfo>
  initialize: () => Promise<boolean>
  start: () => Promise<boolean>
  stop: () => Promise<void>
  getState: () => Promise<string>
  loadConfig: (name?: string) => Promise<boolean>
  saveConfig: (name?: string) => Promise<boolean>
  getConfigJson: () => Promise<string>
  setConfigJson: (json: string) => Promise<boolean>
  validateConfig: (json: string) => Promise<string>
  joystickConnected: () => Promise<boolean>
  getCaptureMode: () => Promise<string>
  keyboardAccessibility: () => Promise<boolean>
  getActiveProfile: () => Promise<string>
  listProfiles: () => Promise<string>
  switchProfile: (name: string) => Promise<boolean>
  createProfile: (name: string, copyCurrent?: boolean) => Promise<boolean>
  deleteProfile: (name: string) => Promise<boolean>
}

const api: InputFlowApi = {
  getBootstrap: () => ipcRenderer.invoke('app:getBootstrap'),
  initialize: () => ipcRenderer.invoke('app:initialize'),
  start: () => ipcRenderer.invoke('app:start'),
  stop: () => ipcRenderer.invoke('app:stop'),
  getState: () => ipcRenderer.invoke('app:state'),
  loadConfig: (name) => ipcRenderer.invoke('app:loadConfig', name ?? 'default'),
  saveConfig: (name) => ipcRenderer.invoke('app:saveConfig', name ?? 'default'),
  getConfigJson: () => ipcRenderer.invoke('app:getConfigJson'),
  setConfigJson: (json) => ipcRenderer.invoke('app:setConfigJson', json),
  validateConfig: (json) => ipcRenderer.invoke('app:validateConfig', json),
  joystickConnected: () => ipcRenderer.invoke('app:joystickConnected'),
  getCaptureMode: () => ipcRenderer.invoke('app:getCaptureMode'),
  keyboardAccessibility: () => ipcRenderer.invoke('app:keyboardAccessibility'),
  getActiveProfile: () => ipcRenderer.invoke('app:getActiveProfile'),
  listProfiles: () => ipcRenderer.invoke('app:listProfiles'),
  switchProfile: (name) => ipcRenderer.invoke('app:switchProfile', name),
  createProfile: (name, copyCurrent) =>
    ipcRenderer.invoke('app:createProfile', name, copyCurrent ?? false),
  deleteProfile: (name) => ipcRenderer.invoke('app:deleteProfile', name)
}

contextBridge.exposeInMainWorld('inputflow', api)
