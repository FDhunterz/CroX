import './StatusBar.css'

interface Props {
  state: string
  joystickConnected: boolean
  onStart: () => void
  onStop: () => void
}

export function StatusBar({ state, joystickConnected, onStart, onStop }: Props) {
  const running = state === 'running'
  return (
    <div className="status-bar">
      <div className="status-group">
        <span className={`status-dot ${running ? 'on' : ''}`} />
        <span>{running ? 'Aktif' : 'Berhenti'}</span>
      </div>
      <div className="status-group">
        <span className={`status-dot ${joystickConnected ? 'on' : ''}`} />
        <span>{joystickConnected ? 'Joystick OK' : 'Joystick ?'}</span>
      </div>
      <button type="button" className="primary" onClick={onStart} disabled={running}>
        Start
      </button>
      <button type="button" className="danger" onClick={onStop} disabled={!running}>
        Stop
      </button>
    </div>
  )
}
