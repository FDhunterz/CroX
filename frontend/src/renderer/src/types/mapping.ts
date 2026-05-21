export type JoystickButton =
  | 'A' | 'B' | 'X' | 'Y'
  | 'LB' | 'RB' | 'LT' | 'RT'
  | 'DPAD_UP' | 'DPAD_DOWN' | 'DPAD_LEFT' | 'DPAD_RIGHT'
  | 'START' | 'BACK' | 'L3' | 'R3'

export type ModifierName = 'Ctrl' | 'Shift' | 'Alt' | 'Meta'

export interface CameraControlSettings {
  enabled: boolean
  stick: 'left' | 'right'
  requireRightButton: boolean
  sensitivity: number
  deadzone: number
  invertY: boolean
}

export const DEFAULT_CAMERA: CameraControlSettings = {
  enabled: false,
  stick: 'right',
  requireRightButton: true,
  sensitivity: 14,
  deadzone: 0.15,
  invertY: true
}

export interface JoystickMapping {
  id: string
  buttons: JoystickButton[]
  joystickButton?: JoystickButton
  key: string
  modifiers: ModifierName[]
  enabled: boolean
}

export interface MappingProfile {
  id: string
  name: string
  mappings: JoystickMapping[]
  exclusiveCapture: boolean
  camera?: CameraControlSettings
}

export interface ValidationError {
  field: string
  message: string
}

export const JOYSTICK_BUTTONS: JoystickButton[] = [
  'A', 'B', 'X', 'Y', 'LB', 'RB', 'LT', 'RT',
  'DPAD_UP', 'DPAD_DOWN', 'DPAD_LEFT', 'DPAD_RIGHT',
  'START', 'BACK', 'L3', 'R3'
]

export const MODIFIERS: ModifierName[] = ['Ctrl', 'Shift', 'Alt', 'Meta']

export const DEFAULT_PROFILE: MappingProfile = {
  id: 'default',
  name: 'Default Joystick Mapping',
  exclusiveCapture: false,
  camera: { ...DEFAULT_CAMERA },
  mappings: [
    {
      id: 'lb-alt-shift',
      buttons: ['LB'],
      key: '',
      modifiers: ['Alt', 'Shift'],
      enabled: true
    },
    {
      id: 'rb-ctrl-shift',
      buttons: ['RB'],
      key: '',
      modifiers: ['Ctrl', 'Shift'],
      enabled: true
    }
  ]
}

export function newMappingId(): string {
  return `map-${Date.now().toString(36)}`
}

export function normalizeMapping(m: JoystickMapping): JoystickMapping {
  const legacy = (m as { joystickButton?: JoystickButton }).joystickButton
  const buttons =
    m.buttons?.length > 0 ? [...m.buttons] : legacy ? [legacy] : []
  return {
    ...m,
    buttons,
    joystickButton: buttons[0],
    enabled: m.enabled !== false
  }
}

/** Buang baris duplikat (signature tombol sama) — hindari profil membengkak. */
export function dedupeMappings(mappings: JoystickMapping[]): JoystickMapping[] {
  const seen = new Set<string>()
  const out: JoystickMapping[] = []
  for (const raw of mappings) {
    const m = normalizeMapping(raw)
    if (m.buttons.length === 0) {
      out.push(m)
      continue
    }
    const sig = comboSignature(m.buttons)
    if (seen.has(sig)) continue
    seen.add(sig)
    out.push(m)
  }
  return out
}

export function normalizeProfile(profile: MappingProfile): MappingProfile {
  return {
    ...profile,
    exclusiveCapture: false,
    camera: { ...DEFAULT_CAMERA, ...profile.camera },
    mappings: dedupeMappings(profile.mappings.map(normalizeMapping))
  }
}

const DPAD_SHORT_LABELS: Partial<Record<JoystickButton, string>> = {
  DPAD_UP: 'D↑',
  DPAD_DOWN: 'D↓',
  DPAD_LEFT: 'D←',
  DPAD_RIGHT: 'D→'
}

/** Label singkat untuk UI (hindari overflow pada DPAD_*). */
export function formatButtonLabel(btn: JoystickButton): string {
  return DPAD_SHORT_LABELS[btn] ?? btn
}

export function formatCombo(buttons: JoystickButton[]): string {
  return buttons.map(formatButtonLabel).join(' + ')
}

export function comboSignature(buttons: JoystickButton[]): string {
  return [...buttons].sort().join('+')
}

export function allUsedButtons(mappings: JoystickMapping[]): Set<JoystickButton> {
  const used = new Set<JoystickButton>()
  for (const m of mappings) {
    for (const b of normalizeMapping(m).buttons) used.add(b)
  }
  return used
}

export function validateMappings(mappings: JoystickMapping[]): ValidationError[] {
  const errors: ValidationError[] = []
  const seenButtons = new Set<JoystickButton>()
  const seenCombos = new Set<string>()
  const seenOutputs = new Set<string>()

  for (const raw of mappings) {
    const m = normalizeMapping(raw)
    if (!m.enabled) continue

    if (m.buttons.length === 0) {
      errors.push({ field: 'buttons', message: `Pemetaan ${m.id} butuh minimal satu tombol` })
      continue
    }

    const cSig = comboSignature(m.buttons)
    if (seenCombos.has(cSig)) {
      errors.push({ field: 'buttons', message: 'Kombinasi tombol sudah dipetakan' })
    }
    seenCombos.add(cSig)

    for (const btn of m.buttons) {
      if (seenButtons.has(btn)) {
        errors.push({
          field: 'buttons',
          message: `Tombol ${btn} sudah dipakai di pemetaan lain`
        })
      }
      seenButtons.add(btn)
    }

    const outKey = `${m.key}|${m.modifiers.sort().join('+')}`
    if (!m.key && m.modifiers.length === 0) {
      errors.push({ field: 'output', message: `Pemetaan ${m.id} butuh key atau modifier` })
      continue
    }
    if (seenOutputs.has(outKey)) {
      errors.push({
        field: 'output',
        message: `Output "${m.key || '(modifier)'}" + [${m.modifiers.join(',')}] sudah dipakai`
      })
    }
    seenOutputs.add(outKey)
  }
  return errors
}
