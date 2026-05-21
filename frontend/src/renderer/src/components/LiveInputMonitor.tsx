import type { JoystickButton } from '../types/mapping'
import { useGamepadLiveState } from '../hooks/useGamepadLiveState'
import './LiveInputMonitor.css'

/** Lingkaran analog — ukuran tetap 1:1 (px). */
const STICK_SIZE_PX = 112
const STICK_DOT_PX = 16
/** Jarak pusat → tepi saat axis = ±1 */
const STICK_TRAVEL_PX = STICK_SIZE_PX / 2 - STICK_DOT_PX / 2

function stickTransform(x: number, y: number): string {
  const clamp = (v: number) => Math.max(-1, Math.min(1, v))
  const dx = clamp(x) * STICK_TRAVEL_PX
  const dy = clamp(y) * STICK_TRAVEL_PX
  return `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`
}

const GRID_BUTTONS: JoystickButton[] = [
  'A',
  'B',
  'X',
  'Y',
  'LB',
  'RB',
  'START',
  'BACK'
]

export function LiveInputMonitor() {
  const live = useGamepadLiveState()

  return (
    <div className="glass-card live-input-card">
      <h3 className="card-section-title">Live Input Monitor</h3>

      <div className="input-grid">
        {GRID_BUTTONS.map((btn) => {
          const active = live.pressed.has(btn)
          return (
            <div key={btn} className={`input-pad ${active ? 'active' : ''}`}>
              {btn.replace('DPAD_', 'D-')}
            </div>
          )
        })}
      </div>

      <div className="trigger-section">
        <div className="trigger-row">
          <span className="trigger-label">LT</span>
          <div className="trigger-bar">
            <div className="trigger-fill" style={{ width: `${live.lt * 100}%` }} />
          </div>
        </div>
        <div className="trigger-row">
          <span className="trigger-label">RT</span>
          <div className="trigger-bar">
            <div className="trigger-fill" style={{ width: `${live.rt * 100}%` }} />
          </div>
        </div>
      </div>

      <div className="sticks-section">
        <div className="stick-unit">
          <span className="stick-label">LS</span>
          <div
            className="stick-visual"
            style={{ width: STICK_SIZE_PX, height: STICK_SIZE_PX }}
          >
            <div className="stick-dot" style={{ transform: stickTransform(live.lx, live.ly) }} />
          </div>
        </div>
        <div className="stick-unit">
          <span className="stick-label">RS</span>
          <div
            className="stick-visual"
            style={{ width: STICK_SIZE_PX, height: STICK_SIZE_PX }}
          >
            <div className="stick-dot" style={{ transform: stickTransform(live.rx, live.ry) }} />
          </div>
        </div>
      </div>

      {!live.connected && (
        <p className="live-hint">Hubungkan controller untuk umpan balik langsung</p>
      )}
    </div>
  )
}
