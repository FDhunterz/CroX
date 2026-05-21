import { useEffect, useRef, useState } from 'react'
import type { MappingProfile } from '../types/mapping'
import { parseConfigImport, serializeConfigExport } from '../utils/configTransfer'
import './ConfigTransfer.css'
import './ProfileManager.css'

interface Props {
  profile: MappingProfile
  applyDisabled?: boolean
  onApplyConfig: (profile: MappingProfile) => Promise<boolean>
  onMessage: (msg: string) => void
}

export function ConfigTransfer({ profile, applyDisabled, onApplyConfig, onMessage }: Props) {
  const [showApplyDialog, setShowApplyDialog] = useState(false)
  const [pasteText, setPasteText] = useState('')
  const [applying, setApplying] = useState(false)
  const textareaRef = useRef<HTMLTextAreaElement>(null)

  useEffect(() => {
    if (showApplyDialog) {
      setPasteText('')
      queueMicrotask(() => textareaRef.current?.focus())
    }
  }, [showApplyDialog])

  const handleCopy = async () => {
    try {
      const text = serializeConfigExport(profile)
      await navigator.clipboard.writeText(text)
      onMessage(`Config disalin (${profile.mappings.length} mapping)`)
    } catch {
      onMessage('Gagal menyalin ke clipboard')
    }
  }

  const handlePasteFromClipboard = async () => {
    try {
      const text = await navigator.clipboard.readText()
      if (!text.trim()) {
        onMessage('Clipboard kosong')
        return
      }
      setPasteText(text)
      onMessage('Ditempel dari clipboard')
    } catch {
      onMessage('Tidak bisa membaca clipboard — tempel manual (Cmd+V)')
    }
  }

  const handleApply = async () => {
    setApplying(true)
    try {
      const imported = parseConfigImport(pasteText)
      const ok = await onApplyConfig(imported)
      if (ok) {
        setShowApplyDialog(false)
        setPasteText('')
      }
    } catch (e) {
      const msg = e instanceof Error ? e.message : 'JSON tidak valid'
      onMessage(`Gagal menerapkan config: ${msg}`)
    } finally {
      setApplying(false)
    }
  }

  return (
    <>
      <div className="config-transfer">
        <button
          type="button"
          className="profile-btn config"
          onClick={() => void handleCopy()}
          title="Salin pemetaan profil aktif ke clipboard (JSON)"
        >
          Salin config
        </button>
        <button
          type="button"
          className="profile-btn config apply"
          disabled={applyDisabled}
          onClick={() => setShowApplyDialog(true)}
          title="Tempel config dari clipboard ke form profil"
        >
          Terapkan config
        </button>
      </div>

      {showApplyDialog && (
        <div
          className="profile-dialog-backdrop"
          role="presentation"
          onClick={() => !applying && setShowApplyDialog(false)}
        >
          <div
            className="profile-dialog config-apply-dialog"
            role="dialog"
            aria-labelledby="apply-config-title"
            onClick={(e) => e.stopPropagation()}
          >
            <h3 id="apply-config-title">Terapkan config</h3>
            <p className="profile-dialog-hint">
              Tempel JSON dari <strong>Salin config</strong> (atau profil mentah dengan{' '}
              <code>mappings</code>). Mengganti isi form profil aktif.
            </p>
            <textarea
              ref={textareaRef}
              className="config-apply-textarea"
              placeholder='{"version":1,"profile":{...}}'
              value={pasteText}
              disabled={applying}
              spellCheck={false}
              onChange={(e) => setPasteText(e.target.value)}
            />
            <div className="config-apply-toolbar">
              <button
                type="button"
                className="profile-dialog-btn"
                disabled={applying}
                onClick={() => void handlePasteFromClipboard()}
              >
                Tempel dari clipboard
              </button>
            </div>
            <div className="profile-dialog-actions">
              <button
                type="button"
                className="profile-dialog-btn"
                disabled={applying}
                onClick={() => setShowApplyDialog(false)}
              >
                Batal
              </button>
              <button
                type="button"
                className="profile-dialog-btn primary"
                disabled={applying || !pasteText.trim()}
                onClick={() => void handleApply()}
              >
                {applying ? 'Menerapkan…' : 'Terapkan'}
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  )
}
