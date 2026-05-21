import type { JoystickButton } from '../types/mapping'

export const BUTTON_COLORS: Partial<Record<JoystickButton, string>> = {
  A: '#4D7CFE',
  B: '#FF5D73',
  X: '#4D7CFE',
  Y: '#F5C542',
  LB: '#64748B',
  RB: '#64748B',
  LT: '#64748B',
  RT: '#64748B',
  START: '#94A3B8',
  BACK: '#94A3B8',
  L3: '#94A3B8',
  R3: '#94A3B8',
  DPAD_UP: '#94A3B8',
  DPAD_DOWN: '#94A3B8',
  DPAD_LEFT: '#94A3B8',
  DPAD_RIGHT: '#94A3B8'
}

export function buttonAccent(btn: JoystickButton): string {
  return BUTTON_COLORS[btn] ?? '#4D7CFE'
}
