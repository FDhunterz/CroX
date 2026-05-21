import type { InputFlowApi } from '../../../preload/index'

declare global {
  interface Window {
    inputflow: InputFlowApi
  }
}

export {}
