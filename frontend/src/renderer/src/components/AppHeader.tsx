import './AppHeader.css'

interface Props {
  engineState: string
  joystickConnected: boolean
  deviceName: string
  onStart: () => void
  onStop: () => void
}

export function AppHeader({
  engineState,
  joystickConnected,
  deviceName,
  onStart,
  onStop
}: Props) {
  const running = engineState === 'running'

  return (
    <header className="app-header">
      <div className="header-left">
        <div className="logo-mark" aria-hidden>
          <svg width="28" height="28" viewBox="0 0 28 28" fill="none">
            <path
              d="M8 14h12M14 8v12"
              stroke="currentColor"
              strokeWidth="2.2"
              strokeLinecap="round"
            />
            <circle cx="14" cy="14" r="11" stroke="currentColor" strokeWidth="1.5" opacity="0.35" />
          </svg>
        </div>
        <div>
          <h1 className="header-title">InputFlow</h1>
          <p className="header-subtitle">Joystick → Keyboard</p>
        </div>
      </div>

      <div className="device-card">
        <span className={`status-dot ${joystickConnected ? 'online' : ''}`} />
        <div className="device-text">
          <span className="device-label">{joystickConnected ? 'Connected' : 'Disconnected'}</span>
          <span className="device-name">{deviceName}</span>
        </div>
      </div>

      <div className="header-actions">
        <button
          type="button"
          className="btn-start"
          onClick={onStart}
          disabled={running}
        >
          <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor" aria-hidden>
            <path d="M8 5v14l11-7z" />
          </svg>
          Start
        </button>
        <button
          type="button"
          className="btn-stop"
          onClick={onStop}
          disabled={!running}
        >
          <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor" aria-hidden>
            <rect x="6" y="6" width="12" height="12" rx="1.5" />
          </svg>
          Stop
        </button>
      </div>
    </header>
  )
}
