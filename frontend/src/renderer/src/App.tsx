import { useCallback, useEffect, useState } from 'react'
import { AppHeader } from './components/AppHeader'
import { ConfigTransfer } from './components/ConfigTransfer'
import { ProfileManager } from './components/ProfileManager'
import { ControllerPanel } from './components/ControllerPanel'
import { LiveInputMonitor } from './components/LiveInputMonitor'
import { MappingEditor } from './components/MappingEditor'
import {
  DEFAULT_PROFILE,
  normalizeProfile,
  type MappingProfile,
  type ValidationError,
  validateMappings
} from './types/mapping'
import { useGamepadLiveState } from './hooks/useGamepadLiveState'
import './styles/app.css'

const TOAST_DISMISS_MS = 5000

export default function App() {
  const [profile, setProfile] = useState<MappingProfile>(DEFAULT_PROFILE)
  const [errors, setErrors] = useState<ValidationError[]>([])
  const [engineState, setEngineState] = useState('stopped')
  const [joystickOk, setJoystickOk] = useState(false)
  const [message, setMessage] = useState('')
  const [configReady, setConfigReady] = useState(false)
  const liveGamepad = useGamepadLiveState()

  const refreshState = useCallback(async () => {
    if (!window.inputflow) return
    setEngineState(await window.inputflow.getState())
    setJoystickOk(await window.inputflow.joystickConnected())
  }, [])

  useEffect(() => {
    const boot = async () => {
      if (!window.inputflow) {
        setMessage('Mode demo — modul native belum terpasang')
        setConfigReady(false)
        return
      }
      const ok = await window.inputflow.initialize()
      setConfigReady(ok)
      if (!ok) {
        setMessage('Gagal menginisialisasi backend profil')
        return
      }
      try {
        const json = await window.inputflow.getConfigJson()
        const parsed = normalizeProfile(JSON.parse(json) as MappingProfile)
        if (parsed.mappings) setProfile(parsed)
      } catch {
        await window.inputflow.setConfigJson(JSON.stringify(DEFAULT_PROFILE))
        setProfile(DEFAULT_PROFILE)
      }
      await refreshState()
    }
    void boot()
    const t = setInterval(refreshState, 1500)
    return () => clearInterval(t)
  }, [refreshState])

  useEffect(() => {
    if (!message) return
    const t = setTimeout(() => setMessage(''), TOAST_DISMISS_MS)
    return () => clearTimeout(t)
  }, [message])

  const applyProfile = async (next: MappingProfile) => {
    const validation = validateMappings(next.mappings)
    setErrors(validation)
    if (validation.length) {
      setMessage('Perbaiki pemetaan duplikat sebelum menerapkan')
      return false
    }
    const normalized = normalizeProfile(next)
    setProfile(normalized)
    if (window.inputflow) {
      const ok = await window.inputflow.setConfigJson(JSON.stringify(normalized))
      if (!ok) {
        setMessage('Gagal menyimpan ke backend')
        return false
      }
    }
    setMessage('Pemetaan diperbarui')
    return true
  }

  const handleSave = async () => {
    if (!(await applyProfile(profile))) return
    if (window.inputflow) {
      const name = await window.inputflow.getActiveProfile()
      await window.inputflow.saveConfig(name)
      setMessage(`Disimpan ke profil "${name}"`)
    }
  }

  const handleStart = async () => {
    const validation = validateMappings(profile.mappings)
    setErrors(validation)
    if (validation.length) {
      setMessage('Tidak bisa start — ada pemetaan duplikat')
      return
    }
    if (!window.inputflow) {
      setMessage('Native module tidak tersedia')
      return
    }
    await window.inputflow.setConfigJson(JSON.stringify(normalizeProfile(profile)))
    const axOk = window.inputflow ? await window.inputflow.keyboardAccessibility() : false
    if (!axOk) {
      setMessage(
        'Izin Accessibility diperlukan — aktifkan InputFlow di System Settings, lalu Start lagi.'
      )
      return
    }
    const ok = await window.inputflow.start()
    setJoystickOk(window.inputflow ? await window.inputflow.joystickConnected() : false)
    setEngineState(window.inputflow ? await window.inputflow.getState() : 'stopped')
    setMessage(
      ok
        ? 'Mapping aktif (mode bersama)'
        : 'Gagal start — sambungkan joystick lalu coba lagi'
    )
  }

  const handleStop = async () => {
    if (window.inputflow) {
      const validation = validateMappings(profile.mappings)
      if (validation.length === 0) {
        await window.inputflow.setConfigJson(JSON.stringify(normalizeProfile(profile)))
      }
      await window.inputflow.stop()
    }
    setMessage('Mapping dihentikan')
    await refreshState()
  }

  const deviceName = joystickOk
    ? liveGamepad.connected
      ? liveGamepad.name
      : 'Controller Connected'
    : liveGamepad.connected
      ? liveGamepad.name
      : 'No Controller'

  const isErrorToast = message.includes('Gagal') || message.includes('Perbaiki') || message.includes('diperlukan')

  return (
    <div className="app-shell">
      <AppHeader
        engineState={engineState}
        joystickConnected={joystickOk || liveGamepad.connected}
        deviceName={deviceName}
        onStart={handleStart}
        onStop={handleStop}
      />

      {message && (
        <div className="toast-stack">
          <div className={`toast ${isErrorToast ? 'error' : 'success'}`}>{message}</div>
        </div>
      )}

      <div className="profile-bar-wrap">
        <div className="profile-bar-row">
          <ProfileManager
            configReady={configReady}
            disabled={engineState === 'running'}
            onProfileLoaded={(p) => {
              setErrors([])
              setProfile(normalizeProfile(p))
            }}
            onMessage={setMessage}
          />
          <ConfigTransfer
            profile={profile}
            applyDisabled={engineState === 'running'}
            onApplyConfig={async (next) => {
              setErrors([])
              const ok = await applyProfile(next)
              if (ok) setMessage(`Config diterapkan (${next.mappings.length} mapping)`)
              return ok
            }}
            onMessage={setMessage}
          />
        </div>
      </div>

      <main className="app-body">
        <aside className="left-panel">
          <ControllerPanel
            connected={joystickOk || liveGamepad.connected}
            deviceName={deviceName}
            engineRunning={engineState === 'running'}
          />
          <LiveInputMonitor />
        </aside>

        <section className="right-panel">
          <MappingEditor
            profile={profile}
            onChange={setProfile}
            onSave={handleSave}
            errors={errors}
            editingLocked={engineState === 'running'}
          />
        </section>
      </main>
    </div>
  )
}
