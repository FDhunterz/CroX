import { useEffect, useState } from 'react'
import type { JoystickButton } from '../types/mapping'
import { gamepadIndexToButton, getActiveGamepad, isButtonPressed } from '../utils/gamepad'

export interface GamepadLiveState {
  connected: boolean
  name: string
  pressed: Set<JoystickButton>
  lt: number
  rt: number
  lx: number
  ly: number
  rx: number
  ry: number
}

const EMPTY: GamepadLiveState = {
  connected: false,
  name: 'No controller',
  pressed: new Set(),
  lt: 0,
  rt: 0,
  lx: 0,
  ly: 0,
  rx: 0,
  ry: 0
}

export function useGamepadLiveState(): GamepadLiveState {
  const [state, setState] = useState<GamepadLiveState>(EMPTY)

  useEffect(() => {
    let raf = 0
    const tick = () => {
      const gp = getActiveGamepad()
      if (!gp) {
        setState(EMPTY)
      } else {
        const pressed = new Set<JoystickButton>()
        for (let i = 0; i < gp.buttons.length; i++) {
          if (isButtonPressed(gp.buttons[i])) {
            const b = gamepadIndexToButton(i, gp)
            if (b) pressed.add(b)
          }
        }
        setState({
          connected: true,
          name: gp.id.replace(/\s*\([^)]*\)\s*/g, '').trim() || 'Game Controller',
          pressed,
          lt: gp.buttons[6]?.value ?? 0,
          rt: gp.buttons[7]?.value ?? 0,
          lx: gp.axes[0] ?? 0,
          ly: gp.axes[1] ?? 0,
          rx: gp.axes[2] ?? 0,
          ry: gp.axes[3] ?? 0
        })
      }
      raf = requestAnimationFrame(tick)
    }
    raf = requestAnimationFrame(tick)
    return () => cancelAnimationFrame(raf)
  }, [])

  return state
}
