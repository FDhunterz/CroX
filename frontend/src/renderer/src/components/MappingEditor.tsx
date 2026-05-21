import { useRef, useState, type Dispatch, type SetStateAction } from 'react'
import {
  DEFAULT_CAMERA,
  JOYSTICK_BUTTONS,
  MODIFIERS,
  allUsedButtons,
  formatButtonLabel,
  formatCombo,
  newMappingId,
  normalizeMapping,
  type CameraControlSettings,
  type JoystickButton,
  type JoystickMapping,
  type MappingProfile,
  type ValidationError
} from '../types/mapping'
import { buttonAccent } from '../utils/buttonColors'
import { useJoystickCapture } from '../hooks/useJoystickCapture'
import { JoystickCaptureOverlay } from './JoystickCaptureOverlay'
import './MappingEditor.css'

interface Props {
  profile: MappingProfile
  onChange: Dispatch<SetStateAction<MappingProfile>>
  onSave: () => void
  errors: ValidationError[]
  /** Saat mapping engine jalan — jangan edit key (hindari injeksi keyboard sintetis). */
  editingLocked?: boolean
}

function formatKeyDisplay(m: JoystickMapping): string {
  const n = normalizeMapping(m)
  if (n.key) return n.key
  if (n.modifiers.length) return n.modifiers.join(' + ')
  return '—'
}

export function MappingEditor({
  profile,
  onChange,
  onSave,
  errors,
  editingLocked = false
}: Props) {
  const { active, hint, startCapture, stopCapture } = useJoystickCapture()
  const pendingIndexRef = useRef<number | null>(null)
  const pendingIsNewRef = useRef(false)
  const [comboDraft, setComboDraft] = useState<JoystickButton[]>([])
  const [comboMode, setComboMode] = useState(false)
  const comboModeRef = useRef(false)

  const usedButtons = () => {
    const used = allUsedButtons(profile.mappings)
    const idx = pendingIndexRef.current
    if (idx !== null && !pendingIsNewRef.current) {
      for (const b of normalizeMapping(profile.mappings[idx]).buttons) {
        used.delete(b)
      }
    }
    for (const b of comboDraft) used.delete(b)
    return used
  }

  const updateMapping = (index: number, patch: Partial<JoystickMapping>) => {
    onChange((prev) => ({
      ...prev,
      mappings: prev.mappings.map((m, i) =>
        i === index ? normalizeMapping({ ...m, ...patch }) : normalizeMapping(m)
      )
    }))
  }

  const finishComboCapture = () => {
    if (comboDraft.length === 0) {
      comboModeRef.current = false
      stopCapture()
      setComboMode(false)
      return
    }
    const buttons = [...comboDraft]
    if (pendingIsNewRef.current || pendingIndexRef.current === null) {
      onChange((prev) => ({
        ...prev,
        mappings: [
          ...prev.mappings,
          {
            id: newMappingId(),
            buttons,
            key: '',
            modifiers: [],
            enabled: true
          }
        ]
      }))
    } else {
      updateMapping(pendingIndexRef.current, { buttons })
    }
    pendingIndexRef.current = null
    pendingIsNewRef.current = false
    setComboDraft([])
    comboModeRef.current = false
    setComboMode(false)
    stopCapture()
  }

  const cancelCapture = () => {
    pendingIndexRef.current = null
    pendingIsNewRef.current = false
    setComboDraft([])
    comboModeRef.current = false
    setComboMode(false)
    stopCapture()
  }

  const onButtonCaptured = (button: JoystickButton) => {
    if (comboModeRef.current) {
      setComboDraft((prev) => {
        if (prev.includes(button)) return prev
        return [...prev, button]
      })
      return
    }
    const buttons = [button]
    if (pendingIsNewRef.current || pendingIndexRef.current === null) {
      onChange((prev) => ({
        ...prev,
        mappings: [
          ...prev.mappings,
          {
            id: newMappingId(),
            buttons,
            key: '',
            modifiers: [],
            enabled: true
          }
        ]
      }))
    } else {
      updateMapping(pendingIndexRef.current, { buttons })
    }
    pendingIndexRef.current = null
    pendingIsNewRef.current = false
  }

  const beginCapture = (
    index: number | null,
    isNew: boolean,
    asCombo: boolean,
    usedOverride?: Set<JoystickButton>
  ) => {
    pendingIndexRef.current = index
    pendingIsNewRef.current = isNew
    const draft =
      index !== null && !isNew ? [...normalizeMapping(profile.mappings[index]).buttons] : []
    comboModeRef.current = asCombo
    setComboMode(asCombo)
    setComboDraft(draft)
    startCapture({
      usedButtons: usedOverride ?? usedButtons(),
      mode: asCombo ? 'accumulate' : 'single',
      onCaptured: ({ button }) => onButtonCaptured(button),
      onCancel: cancelCapture
    })
  }

  const firstFreeButton = (mappings: JoystickMapping[]): JoystickButton => {
    const used = allUsedButtons(mappings)
    for (const b of JOYSTICK_BUTTONS) {
      if (!used.has(b)) return b
    }
    return 'A'
  }

  /** Tambah baris langsung (tanpa wajib gamepad); kombinasi bisa lanjut capture. */
  const addMappingRow = (asCombo = false) => {
    onChange((prev) => {
      const nextIndex = prev.mappings.length
      if (asCombo) {
        pendingIndexRef.current = nextIndex
        pendingIsNewRef.current = false
        queueMicrotask(() => beginCapture(nextIndex, false, true, usedButtons()))
      }
      return {
        ...prev,
        mappings: [
          ...prev.mappings,
          {
            id: newMappingId(),
            buttons: asCombo ? [] : [firstFreeButton(prev.mappings)],
            key: '',
            modifiers: [],
            enabled: true
          }
        ]
      }
    })
  }

  const captureForRow = (index: number) => {
    const isCombo = normalizeMapping(profile.mappings[index]).buttons.length > 1
    beginCapture(index, false, isCombo)
  }

  const removeMapping = (index: number) => {
    onChange((prev) => ({
      ...prev,
      mappings: prev.mappings.filter((_, i) => i !== index)
    }))
  }

  const moveMapping = (index: number, delta: -1 | 1) => {
    onChange((prev) => {
      const next = index + delta
      if (next < 0 || next >= prev.mappings.length) return prev
      const mappings = [...prev.mappings]
      ;[mappings[index], mappings[next]] = [mappings[next], mappings[index]]
      return { ...prev, mappings }
    })
  }

  const toggleModifier = (index: number, mod: (typeof MODIFIERS)[number]) => {
    const m = normalizeMapping(profile.mappings[index])
    const has = m.modifiers.includes(mod)
    const modifiers = has
      ? m.modifiers.filter((x) => x !== mod)
      : [...m.modifiers, mod]
    updateMapping(index, { modifiers })
  }

  const count = profile.mappings.length

  return (
    <section className="mapping-panel">
      <JoystickCaptureOverlay
        visible={active}
        hint={hint}
        comboMode={comboMode}
        comboButtons={comboMode ? comboDraft : undefined}
        onFinishCombo={finishComboCapture}
        onCancel={cancelCapture}
      />

      <div className="mapping-panel-header">
        <div className="mapping-title-group">
          <h2 className="mapping-title">Key Mappings</h2>
          <span className="mapping-badge">{count} Mappings</span>
        </div>
        <div className="mapping-toolbar">
          <button
            type="button"
            className="btn-toolbar primary"
            onClick={() => beginCapture(null, true, false)}
            disabled={active || editingLocked}
            title="Deteksi satu tombol dari gamepad"
          >
            + Tombol tunggal
          </button>
          <button
            type="button"
            className="btn-toolbar primary"
            onClick={() => beginCapture(null, true, true)}
            disabled={active || editingLocked}
            title="Kombinasi beberapa tombol (jeda ≤150 ms), lalu Selesai"
          >
            + Kombinasi
          </button>
          <button type="button" className="btn-toolbar" onClick={onSave}>
            Simpan
          </button>
        </div>
      </div>

      <div className="option-rows">
        <label className="exclusive-row">
          <input
            type="checkbox"
            checked={profile.camera?.enabled ?? false}
            onChange={(e) =>
              onChange((prev) => ({
                ...prev,
                camera: { ...(prev.camera ?? DEFAULT_CAMERA), enabled: e.target.checked }
              }))
            }
          />
          <span>Kamera: right stick + mouse kanan (swipe arah)</span>
        </label>
      </div>

      <p className="mapping-hint">
        Kombinasi: tekan <strong>+ Kombinasi</strong>, lalu tombol satu per satu (mis. RB → A), klik{' '}
        <strong>Selesai</strong>, isi keyboard <strong>M</strong>.
      </p>

      {errors.length > 0 && (
        <div className="mapping-errors">
          {errors.map((e, i) => (
            <p key={i}>{e.message}</p>
          ))}
        </div>
      )}

      <div className={`mapping-scroll ${active ? 'is-capturing' : ''}`}>
        {profile.mappings.map((raw, i) => {
          const m = normalizeMapping(raw)
          const isCombo = m.buttons.length > 1
          const primary = m.buttons[0]
          return (
            <div key={m.id} className={`mapping-row-card ${!m.enabled ? 'disabled' : ''}`}>
              <div
                className={`btn-indicator ${isCombo ? 'combo' : ''}`}
                style={{
                  background: `${buttonAccent(primary)}22`,
                  borderColor: `${buttonAccent(primary)}55`,
                  color: buttonAccent(primary)
                }}
                title={m.buttons.join(' + ')}
              >
                {m.buttons.length === 0
                  ? '…'
                  : isCombo
                    ? formatCombo(m.buttons)
                    : formatButtonLabel(m.buttons[0])}
              </div>

              <span className="mapping-arrow" aria-hidden>
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                  <path d="M5 12h14M13 6l6 6-6 6" />
                </svg>
              </span>

              <input
                className="key-input"
                value={m.key}
                placeholder="Key (A, M, SPACE…)"
                readOnly={editingLocked}
                title={
                  editingLocked
                    ? 'Hentikan mapping dulu untuk mengedit key'
                    : undefined
                }
                autoComplete="off"
                spellCheck={false}
                onKeyDown={(e) => e.stopPropagation()}
                onChange={(e) => {
                  if (editingLocked) return
                  updateMapping(i, { key: e.target.value.toUpperCase() })
                }}
              />

              <div className="modifier-pills">
                {MODIFIERS.map((mod) => (
                  <button
                    key={mod}
                    type="button"
                    className={m.modifiers.includes(mod) ? 'pill active' : 'pill'}
                    onClick={() => toggleModifier(i, mod)}
                    disabled={active || editingLocked}
                  >
                    {mod}
                  </button>
                ))}
              </div>

              <label className="enabled-chip" title="Aktif">
                <input
                  type="checkbox"
                  checked={m.enabled}
                  onChange={(e) => updateMapping(i, { enabled: e.target.checked })}
                />
                <span>{m.enabled ? 'On' : 'Off'}</span>
              </label>

              <div className="row-actions">
                <button
                  type="button"
                  className="icon-btn"
                  onClick={() => moveMapping(i, -1)}
                  disabled={active || editingLocked || i === 0}
                  title="Geser ke atas"
                  aria-label="Geser ke atas"
                >
                  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M12 19V5M5 12l7-7 7 7" />
                  </svg>
                </button>
                <button
                  type="button"
                  className="icon-btn"
                  onClick={() => moveMapping(i, 1)}
                  disabled={active || editingLocked || i === profile.mappings.length - 1}
                  title="Geser ke bawah"
                  aria-label="Geser ke bawah"
                >
                  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M12 5v14M5 12l7 7 7-7" />
                  </svg>
                </button>
                <button
                  type="button"
                  className="icon-btn"
                  onClick={() => captureForRow(i)}
                  disabled={active || editingLocked}
                  title={isCombo ? 'Ubah kombinasi' : 'Ubah tombol'}
                >
                  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M11 4H4a2 2 0 00-2 2v14a2 2 0 002 2h14a2 2 0 002-2v-7" />
                    <path d="M18.5 2.5a2.12 2.12 0 013 3L12 15l-4 1 1-4 9.5-9.5z" />
                  </svg>
                </button>
                <button
                  type="button"
                  className="icon-btn danger"
                  onClick={() => removeMapping(i)}
                  disabled={active || editingLocked}
                  title="Hapus"
                  aria-label="Hapus mapping"
                >
                  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M3 6h18M8 6V4h8v2M19 6l-1 14H6L5 6" />
                  </svg>
                </button>
              </div>

              <span className="key-preview">→ {formatKeyDisplay(m)}</span>
            </div>
          )
        })}

        {profile.mappings.length === 0 && (
          <button type="button" className="empty-mapping-card" onClick={() => addMappingRow(false)}>
            <div className="empty-icon">
              <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8">
                <path d="M12 5v14M5 12h14" />
              </svg>
            </div>
            <span className="empty-title">Add new mapping</span>
            <span className="empty-sub">
              Gunakan Kombinasi untuk RB + A → M, atau tombol tunggal
            </span>
          </button>
        )}
      </div>
    </section>
  )
}
