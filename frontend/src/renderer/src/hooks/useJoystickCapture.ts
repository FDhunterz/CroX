import { useCallback, useEffect, useRef, useState } from 'react'
import type { JoystickButton } from '../types/mapping'
import {
  COMBO_CAPTURE_WINDOW_MS,
  anyButtonPressed,
  blockedCaptureHint,
  createEmptySnapshot,
  detectNewPress,
  gamepadLayoutHint,
  getActiveGamepad,
  snapshotPressed,
  warmupGamepadApi,
  type ButtonStateSnapshot
} from '../utils/gamepad'

export interface CaptureResult {
  button: JoystickButton
}

interface Options {
  usedButtons?: Set<JoystickButton>
  /** accumulate = kombinasi: beberapa tombol dalam jendela waktu, overlay tetap terbuka. */
  mode?: 'single' | 'accumulate'
  onCaptured: (result: CaptureResult) => void
  onCancel?: () => void
}

export function useJoystickCapture() {
  const [active, setActive] = useState(false)
  const [hint, setHint] = useState('')
  const rafRef = useRef<number>(0)
  const snapshotRef = useRef<ButtonStateSnapshot>([])
  const optionsRef = useRef<Options | null>(null)
  const activeRef = useRef(false)
  const armedRef = useRef(false)
  const waitReleaseRef = useRef(false)
  const lastComboPressRef = useRef(0)

  const cancelRaf = useCallback(() => {
    if (rafRef.current) {
      cancelAnimationFrame(rafRef.current)
      rafRef.current = 0
    }
  }, [])

  const stopCapture = useCallback(() => {
    cancelRaf()
    activeRef.current = false
    armedRef.current = false
    waitReleaseRef.current = false
    lastComboPressRef.current = 0
    setActive(false)
    setHint('')
    optionsRef.current = null
    snapshotRef.current = []
  }, [cancelRaf])

  const tick = useCallback(() => {
    const opts = optionsRef.current
    if (!opts || !activeRef.current) return

    warmupGamepadApi()
    const gp = getActiveGamepad()
    if (!gp) {
      setHint('Joystick tidak terdeteksi — sambungkan controller (nonaktifkan Absorb eksklusif jika dipakai)')
      rafRef.current = requestAnimationFrame(tick)
      return
    }

    if (waitReleaseRef.current) {
      if (anyButtonPressed(gp)) {
        snapshotRef.current = snapshotPressed(gp)
        setHint('Lepaskan tombol sebelum tekan berikutnya…')
        rafRef.current = requestAnimationFrame(tick)
        return
      }
      waitReleaseRef.current = false
      snapshotRef.current = createEmptySnapshot(gp)
    }

    if (!armedRef.current) {
      if (anyButtonPressed(gp)) {
        snapshotRef.current = snapshotPressed(gp)
        setHint('Lepaskan semua tombol dulu, lalu tekan untuk mendeteksi…')
        rafRef.current = requestAnimationFrame(tick)
        return
      }
      armedRef.current = true
      snapshotRef.current = createEmptySnapshot(gp)
      const layoutNote = gamepadLayoutHint(gp)
      setHint(
        (opts.mode === 'accumulate'
          ? `Tekan tombol kombinasi (BACK/START diabaikan, jeda ≤ ${COMBO_CAPTURE_WINDOW_MS} ms)…`
          : 'Tekan tombol gamepad (BACK/START/Home diabaikan)…') +
          (layoutNote ? ` ${layoutNote}` : '')
      )
      rafRef.current = requestAnimationFrame(tick)
      return
    }

    const found = detectNewPress(gp, snapshotRef.current)
    snapshotRef.current = snapshotPressed(gp)

    if (found) {
      if (found.kind === 'vendor') {
        waitReleaseRef.current = true
        setHint(`Tombol Home/Guide (#${found.index}) diabaikan — gunakan tombol gamepad saja`)
        rafRef.current = requestAnimationFrame(tick)
        return
      }

      if (found.kind === 'blocked') {
        waitReleaseRef.current = true
        setHint(blockedCaptureHint(found.button))
        rafRef.current = requestAnimationFrame(tick)
        return
      }

      if (opts.usedButtons?.has(found.button)) {
        waitReleaseRef.current = true
        setHint(`Tombol ${found.button} sudah dipakai — tekan tombol lain`)
        rafRef.current = requestAnimationFrame(tick)
        return
      }

      if (opts.mode === 'accumulate') {
        const now = performance.now()
        const gap = now - lastComboPressRef.current
        if (lastComboPressRef.current > 0 && gap > COMBO_CAPTURE_WINDOW_MS) {
          setHint(
            `Jeda terlalu lama (${Math.round(gap)} ms) — tekan Selesai atau lanjut tombol berikutnya`
          )
        }
        lastComboPressRef.current = now
        opts.onCaptured({ button: found.button })
        waitReleaseRef.current = true
        setHint(`${found.button} ditambahkan — tekan tombol lain (≤${COMBO_CAPTURE_WINDOW_MS} ms) atau Selesai`)
        rafRef.current = requestAnimationFrame(tick)
        return
      }

      opts.onCaptured({ button: found.button })
      stopCapture()
      return
    }

    setHint(
      opts.mode === 'accumulate'
        ? `Tekan tombol berikutnya (≤${COMBO_CAPTURE_WINDOW_MS} ms antar tekan) atau klik Selesai`
        : 'Tekan satu tombol di joystick…'
    )
    rafRef.current = requestAnimationFrame(tick)
  }, [stopCapture])

  const startCapture = useCallback(
    (options: Options) => {
      cancelRaf()
      warmupGamepadApi()
      optionsRef.current = options
      snapshotRef.current = []
      armedRef.current = false
      waitReleaseRef.current = false
      lastComboPressRef.current = 0
      activeRef.current = true
      setActive(true)
      setHint('Menunggu joystick…')
      rafRef.current = requestAnimationFrame(tick)
    },
    [cancelRaf, tick]
  )

  useEffect(() => {
    const onGamepad = () => {
      warmupGamepadApi()
      if (activeRef.current) {
        armedRef.current = false
        snapshotRef.current = []
      }
    }
    window.addEventListener('gamepadconnected', onGamepad)
    window.addEventListener('focus', onGamepad)
    return () => {
      window.removeEventListener('gamepadconnected', onGamepad)
      window.removeEventListener('focus', onGamepad)
    }
  }, [])

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && activeRef.current) {
        optionsRef.current?.onCancel?.()
        stopCapture()
      }
    }
    window.addEventListener('keydown', onKey)
    return () => window.removeEventListener('keydown', onKey)
  }, [stopCapture])

  useEffect(() => {
    return () => {
      cancelRaf()
    }
  }, [cancelRaf])

  return { active, hint, startCapture, stopCapture }
}
