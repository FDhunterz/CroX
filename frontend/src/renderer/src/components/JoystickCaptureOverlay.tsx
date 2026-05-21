import type { JoystickButton } from '../types/mapping'
import { formatCombo } from '../types/mapping'
import './JoystickCaptureOverlay.css'

interface Props {
  visible: boolean
  hint: string
  comboButtons?: JoystickButton[]
  comboMode?: boolean
  onFinishCombo?: () => void
  onCancel: () => void
}

export function JoystickCaptureOverlay({
  visible,
  hint,
  comboButtons = [],
  comboMode = false,
  onFinishCombo,
  onCancel
}: Props) {
  if (!visible) return null

  return (
    <div className="capture-overlay" role="dialog" aria-modal="true">
      <div className="capture-card">
        <div className="capture-pulse" aria-hidden>
          <svg width="36" height="36" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
            <path d="M6 12h4v4H6v-4zM14 10h4v6h-4v-6zM4 8a2 2 0 012-2h2v2H6v2H4V8z" opacity="0.5" />
            <rect x="2" y="6" width="20" height="12" rx="4" stroke="currentColor" />
          </svg>
        </div>
        <h3>{comboMode ? 'Kombinasi tombol' : 'Deteksi tombol joystick'}</h3>
        {comboButtons.length > 0 && (
          <div className="capture-combo-preview">{formatCombo(comboButtons)}</div>
        )}
        <p className="capture-hint">{hint}</p>
        <p className="capture-sub">
          {comboMode
            ? 'Tekan tombol satu per satu (mis. RB lalu A, jeda hingga 150 ms), lepaskan tiap tombol sebelum tekan berikutnya, lalu Selesai'
            : 'Tekan satu tombol pada gamepad (lepaskan dulu jika masih menekan tombol lain)'}
        </p>
        <p className="capture-sub capture-sub-muted">
          BACK / START / Home tidak dipetakan. Xbox: X = kiri; PlayStation: ikon bawah = A (bukan X).
        </p>
        <div className="capture-actions">
          {comboMode && onFinishCombo && (
            <button
              type="button"
              className="capture-done"
              disabled={comboButtons.length === 0}
              onClick={onFinishCombo}
            >
              Selesai
            </button>
          )}
          <button type="button" className="capture-cancel" onClick={onCancel}>
            Batal (Esc)
          </button>
        </div>
      </div>
    </div>
  )
}
