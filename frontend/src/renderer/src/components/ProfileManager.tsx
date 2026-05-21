import { useCallback, useEffect, useRef, useState } from 'react'
import type { MappingProfile } from '../types/mapping'
import { normalizeProfile } from '../types/mapping'
import './ProfileManager.css'

interface Props {
  configReady: boolean
  onProfileLoaded: (profile: MappingProfile, activeName: string) => void
  onMessage: (msg: string) => void
  disabled?: boolean
}

function slugify(name: string): string {
  return name
    .trim()
    .toLowerCase()
    .replace(/\s+/g, '-')
    .replace(/[^a-z0-9_-]/g, '')
}

function mergeProfileNames(...sources: string[][]): string[] {
  const set = new Set<string>()
  for (const src of sources) {
    for (const p of src) {
      if (p) set.add(p)
    }
  }
  if (set.size === 0) set.add('default')
  return [...set].sort()
}

export function ProfileManager({
  configReady,
  onProfileLoaded,
  onMessage,
  disabled
}: Props) {
  const [profiles, setProfiles] = useState<string[]>(['default'])
  const [active, setActive] = useState('default')
  const [showNewDialog, setShowNewDialog] = useState(false)
  const [newProfileName, setNewProfileName] = useState('')
  const [creating, setCreating] = useState(false)
  const newInputRef = useRef<HTMLInputElement>(null)

  const refresh = useCallback(async () => {
    if (!window.inputflow || !configReady) return
    try {
      const list = JSON.parse(await window.inputflow.listProfiles()) as string[]
      const act = await window.inputflow.getActiveProfile()
      setProfiles(mergeProfileNames(list))
      setActive(act || list[0] || 'default')
    } catch {
      setProfiles(['default'])
      setActive('default')
    }
  }, [configReady])

  useEffect(() => {
    void refresh()
  }, [refresh])

  useEffect(() => {
    if (showNewDialog) {
      setNewProfileName('')
      queueMicrotask(() => newInputRef.current?.focus())
    }
  }, [showNewDialog])

  const loadActiveIntoUi = async () => {
    if (!window.inputflow) return
    const json = await window.inputflow.getConfigJson()
    const profile = normalizeProfile(JSON.parse(json) as MappingProfile)
    const act = await window.inputflow.getActiveProfile()
    onProfileLoaded(profile, act)
    return act
  }

  const handleSwitch = async (name: string) => {
    if (!window.inputflow || name === active) return
    const ok = await window.inputflow.switchProfile(name)
    if (!ok) {
      onMessage('Gagal memuat profil')
      return
    }
    setActive(name)
    await loadActiveIntoUi()
    onMessage(`Profil aktif: ${name}`)
    await refresh()
  }

  const submitNewProfile = async () => {
    if (!window.inputflow || !configReady) {
      onMessage('Backend belum siap — tunggu sebentar lalu coba lagi')
      return
    }
    const label = newProfileName.trim()
    if (!label) {
      onMessage('Isi nama profil')
      return
    }
    const slug = slugify(label)
    if (!slug) {
      onMessage('Nama profil tidak valid (gunakan huruf/angka)')
      return
    }
    if (slug === 'default' || slug === 'manifest') {
      onMessage('Nama "default" / "manifest" tidak boleh dipakai')
      return
    }

    setCreating(true)
    try {
      const ok = await window.inputflow.createProfile(slug, false)
      if (!ok) {
        onMessage(`Gagal membuat profil "${slug}" — mungkin sudah ada`)
        await refresh()
        return
      }

      setShowNewDialog(false)
      setProfiles((prev) => mergeProfileNames([prev, [slug]]))
      setActive(slug)

      await loadActiveIntoUi()

      const list = JSON.parse(await window.inputflow.listProfiles()) as string[]
      setProfiles(mergeProfileNames(list, [slug]))
      const act = await window.inputflow.getActiveProfile()
      setActive(act || slug)

      onMessage(`Profil "${slug}" dibuat`)
    } finally {
      setCreating(false)
    }
  }

  const handleDelete = async () => {
    if (profiles.length <= 1) {
      onMessage('Minimal harus ada satu profil')
      return
    }
    if (!window.confirm(`Hapus profil "${active}"?`)) return
    if (!window.inputflow) return
    const other = profiles.find((p) => p !== active)
    if (!other) return
    await window.inputflow.switchProfile(other)
    const ok = await window.inputflow.deleteProfile(active)
    if (!ok) {
      onMessage('Gagal menghapus profil')
      await refresh()
      return
    }
    setActive(other)
    await loadActiveIntoUi()
    onMessage(`Profil dihapus — aktif: ${other}`)
    await refresh()
  }

  return (
    <>
      <div className="profile-manager">
        <label className="profile-label">
          Profil
          <select
            value={profiles.includes(active) ? active : profiles[0] ?? 'default'}
            disabled={disabled || !configReady}
            onChange={(e) => void handleSwitch(e.target.value)}
          >
            {profiles.map((p) => (
              <option key={p} value={p}>
                {p}
              </option>
            ))}
          </select>
        </label>
        <button
          type="button"
          className="profile-btn"
          disabled={disabled || !configReady || creating}
          onClick={() => setShowNewDialog(true)}
        >
          + Profil
        </button>
        <button
          type="button"
          className="profile-btn"
          disabled={disabled || !configReady || profiles.length <= 1}
          onClick={() => void handleDelete()}
        >
          Hapus
        </button>
      </div>

      {showNewDialog && (
        <div
          className="profile-dialog-backdrop"
          role="presentation"
          onClick={() => !creating && setShowNewDialog(false)}
        >
          <div
            className="profile-dialog"
            role="dialog"
            aria-labelledby="new-profile-title"
            onClick={(e) => e.stopPropagation()}
          >
            <h3 id="new-profile-title">Profil baru</h3>
            <p className="profile-dialog-hint">
              Nama disimpan sebagai slug (mis. &quot;PvP&quot; → <code>pvp</code>)
            </p>
            <input
              ref={newInputRef}
              className="profile-dialog-input"
              type="text"
              placeholder="mis. dmo-pvp"
              value={newProfileName}
              disabled={creating}
              onChange={(e) => setNewProfileName(e.target.value)}
              onKeyDown={(e) => {
                if (e.key === 'Enter') void submitNewProfile()
                if (e.key === 'Escape' && !creating) setShowNewDialog(false)
              }}
            />
            <div className="profile-dialog-actions">
              <button
                type="button"
                className="profile-dialog-btn"
                disabled={creating}
                onClick={() => setShowNewDialog(false)}
              >
                Batal
              </button>
              <button
                type="button"
                className="profile-dialog-btn primary"
                disabled={creating}
                onClick={() => void submitNewProfile()}
              >
                {creating ? 'Membuat…' : 'Buat profil'}
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  )
}
