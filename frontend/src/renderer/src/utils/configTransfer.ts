import { normalizeProfile, type MappingProfile } from '../types/mapping'

const EXPORT_VERSION = 1

export interface ConfigExportPayload {
  version: number
  exportedAt: string
  profile: MappingProfile
}

export function serializeConfigExport(profile: MappingProfile): string {
  const payload: ConfigExportPayload = {
    version: EXPORT_VERSION,
    exportedAt: new Date().toISOString(),
    profile: normalizeProfile(profile)
  }
  return JSON.stringify(payload, null, 2)
}

export function parseConfigImport(raw: string): MappingProfile {
  const trimmed = raw.trim()
  if (!trimmed) throw new Error('Config kosong')

  const parsed = JSON.parse(trimmed) as unknown
  if (!parsed || typeof parsed !== 'object') {
    throw new Error('Format JSON tidak valid')
  }

  const obj = parsed as Record<string, unknown>
  if (Array.isArray(obj.mappings)) {
    return normalizeProfile(obj as MappingProfile)
  }
  if (obj.profile && typeof obj.profile === 'object') {
    const inner = obj.profile as Record<string, unknown>
    if (Array.isArray(inner.mappings)) {
      return normalizeProfile(inner as MappingProfile)
    }
  }

  throw new Error('Harus berisi objek profile dengan array mappings')
}
