import type { JoystickButton } from '../types/mapping'
import { formatButtonLabel } from '../types/mapping'

/**
 * Pemetaan W3C Standard Gamepad (gamepad.mapping === 'standard').
 * https://w3c.github.io/gamepad/#remapping
 */
const STANDARD_GAMEPAD_INDEX_TO_BUTTON: Record<number, JoystickButton> = {
  0: 'A',
  1: 'B',
  2: 'X',
  3: 'Y',
  4: 'LB',
  5: 'RB',
  6: 'LT',
  7: 'RT',
  8: 'BACK',
  9: 'START',
  10: 'L3',
  11: 'R3',
  12: 'DPAD_UP',
  13: 'DPAD_DOWN',
  14: 'DPAD_LEFT',
  15: 'DPAD_RIGHT'
}

/** Fallback untuk layout lama / non-standar (sama dengan standar untuk 0–15). */
const LEGACY_GAMEPAD_INDEX_TO_BUTTON: Record<number, JoystickButton> = {
  ...STANDARD_GAMEPAD_INDEX_TO_BUTTON
}

/** Tombol sistem — tidak boleh dipilih saat capture (Share, Start, Home, dll.). */
export const CAPTURE_BLOCKED_BUTTONS: ReadonlySet<JoystickButton> = new Set([
  'BACK',
  'START'
])

/** Tombol yang boleh dipetakan saat capture. */
export const CAPTURE_ALLOWED_BUTTONS: readonly JoystickButton[] = [
  'A',
  'B',
  'X',
  'Y',
  'LB',
  'RB',
  'LT',
  'RT',
  'DPAD_UP',
  'DPAD_DOWN',
  'DPAD_LEFT',
  'DPAD_RIGHT',
  'L3',
  'R3'
]

/** Jendela waktu antar tombol untuk mode kombinasi (ms). */
export const COMBO_CAPTURE_WINDOW_MS = 150

const PRESS_THRESHOLD = 0.4

const MAX_STANDARD_BUTTON_INDEX = 15

export function isCaptureBlockedButton(btn: JoystickButton): boolean {
  return CAPTURE_BLOCKED_BUTTONS.has(btn)
}

export function blockedCaptureHint(btn: JoystickButton): string {
  return `Tombol ${formatButtonLabel(btn)} (menu/sistem) tidak dipetakan — gunakan A, B, X, Y, LB, RB, LT, RT, D-pad, L3/R3`
}

export function gamepadIndexToButton(index: number, gamepad?: Gamepad | null): JoystickButton | null {
  if (index < 0 || index > MAX_STANDARD_BUTTON_INDEX) return null
  const table =
    gamepad?.mapping === 'standard'
      ? STANDARD_GAMEPAD_INDEX_TO_BUTTON
      : LEGACY_GAMEPAD_INDEX_TO_BUTTON
  return table[index] ?? null
}

export function isButtonPressed(btn: GamepadButton): boolean {
  return btn.pressed || btn.value > PRESS_THRESHOLD
}

/** Paksa browser refresh daftar gamepad (perlu setelah connect / fokus window). */
export function warmupGamepadApi(): void {
  try {
    if (typeof navigator.getGamepads === 'function') {
      navigator.getGamepads()
    }
  } catch {
    /* ignore */
  }
}

/** Ambil gamepad pertama yang terhubung. */
export function getActiveGamepad(): Gamepad | null {
  warmupGamepadApi()
  const pads = navigator.getGamepads()
  for (const gp of pads) {
    if (gp && gp.connected) return gp
  }
  return null
}

export type ButtonStateSnapshot = boolean[]

export type PressDetectResult =
  | { kind: 'press'; button: JoystickButton; index: number }
  | { kind: 'blocked'; button: JoystickButton; index: number }
  | { kind: 'vendor'; index: number }

export function snapshotPressed(gamepad: Gamepad): ButtonStateSnapshot {
  return gamepad.buttons.map((b) => isButtonPressed(b))
}

export function anyButtonPressed(gamepad: Gamepad): boolean {
  return snapshotPressed(gamepad).some(Boolean)
}

/** Deteksi tombol yang baru ditekan (edge: off → on). */
export function detectNewPress(
  gamepad: Gamepad,
  previous: ButtonStateSnapshot
): PressDetectResult | null {
  for (let i = 0; i < gamepad.buttons.length; i++) {
    const pressed = isButtonPressed(gamepad.buttons[i])
    const was = previous[i] ?? false
    if (!pressed || was) continue

    const mapped = gamepadIndexToButton(i, gamepad)
    if (!mapped) {
      return { kind: 'vendor', index: i }
    }
    if (isCaptureBlockedButton(mapped)) {
      return { kind: 'blocked', button: mapped, index: i }
    }
    return { kind: 'press', button: mapped, index: i }
  }
  return null
}

export function createEmptySnapshot(gamepad: Gamepad): ButtonStateSnapshot {
  return gamepad.buttons.map(() => false)
}

export function gamepadLayoutHint(gamepad: Gamepad | null): string | null {
  if (!gamepad) return null
  if (gamepad.mapping === 'standard') return null
  return 'Layout non-standar — jika tombol salah, nonaktifkan Absorb eksklusif atau sambungkan sebagai Xbox/Standard'
}
