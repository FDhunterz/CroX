import './ControllerPanel.css'

interface Props {
  connected: boolean
  deviceName: string
  engineRunning: boolean
}

export function ControllerPanel({
  connected,
  deviceName,
  engineRunning
}: Props) {
  return (
    <div className="glass-card controller-status-card">
      <h3 className="controller-card-title">Controller Status</h3>

      <div className="controller-meta">
        <div className="meta-row">
          <span className="meta-icon" aria-hidden>
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <rect x="2" y="7" width="18" height="10" rx="2" />
              <path d="M22 11v4" />
            </svg>
          </span>
          <div className="meta-content">
            <span className="meta-label">Status</span>
            <div className="meta-bar track">
              <div
                className="meta-bar-fill success"
                style={{ width: connected ? '100%' : '8%' }}
              />
            </div>
            <span className="meta-value">{connected ? 'Online' : 'Offline'}</span>
          </div>
        </div>
        <div className="meta-row">
          <span className="meta-icon" aria-hidden>
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M12 2v4M12 18v4M4.93 4.93l2.83 2.83M16.24 16.24l2.83 2.83" />
            </svg>
          </span>
          <div className="meta-content">
            <span className="meta-label">Mode</span>
            <div className="meta-bar track">
              <div
                className="meta-bar-fill accent"
                style={{ width: engineRunning ? '72%' : '24%' }}
              />
            </div>
            <span className="meta-value">{engineRunning ? 'Active' : 'Idle'}</span>
          </div>
        </div>
      </div>

      <p className="controller-device-caption">{deviceName}</p>
    </div>
  )
}
