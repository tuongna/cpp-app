import { defineStore } from 'pinia'

// ──────────────────────────────────────────────────────────────────────────────
// Types
// ──────────────────────────────────────────────────────────────────────────────
export type GameStatus =
  | 'idle'
  | 'downloading'
  | 'paused'
  | 'installed'
  | 'launching'
  | 'playing'
  | 'error'

export interface Game {
  id: string
  title: string
  developer: string
  publisher: string
  description: string
  releaseYear: number
  genres: string[]
  rating: number         // 0-100
  sizeMB: number
  headerImage: string    // URL or local path
  backgroundImage: string
  executableName: string // e.g. "game.exe" or "game"
  downloadUrl: string
  installDir?: string
}

export interface GameState {
  id: string
  status: GameStatus
  progressPercentage: number
  downloadSpeed: string
  eta: string
  errorMessage?: string
}

// ──────────────────────────────────────────────────────────────────────────────
// Store
// ──────────────────────────────────────────────────────────────────────────────
export const useGameStore = defineStore('games', {
  state: () => ({
    games: [] as Game[],
    gameStates: {} as Record<string, GameState>,
    selectedGameId: null as string | null,
    activeTab: 'store' as 'store' | 'library' | 'downloads',
    baseInstallDir: 'C:/SteamCloneGames',
    isNativeBridgeReady: false,
  }),

  getters: {
    selectedGame: (state): Game | null =>
      state.games.find(g => g.id === state.selectedGameId) ?? null,

    getGameState: (state) => (id: string): GameState =>
      state.gameStates[id] ?? {
        id,
        status: 'idle',
        progressPercentage: 0,
        downloadSpeed: '',
        eta: '',
      },

    installedGames: (state): Game[] =>
      state.games.filter(g => {
        const s = state.gameStates[g.id]?.status
        return s === 'installed' || s === 'playing' || s === 'launching'
      }),

    downloadingGames: (state): Game[] =>
      state.games.filter(g => {
        const s = state.gameStates[g.id]?.status
        return s === 'downloading' || s === 'paused'
      }),
  },

  actions: {
    initGames(games: Game[]) {
      this.games = games
      // Initialize default states
      for (const game of games) {
        if (!this.gameStates[game.id]) {
          this.gameStates[game.id] = {
            id: game.id,
            status: 'idle',
            progressPercentage: 0,
            downloadSpeed: '',
            eta: '',
          }
        }
      }
    },

    selectGame(id: string) {
      this.selectedGameId = id
    },

    setTab(tab: 'store' | 'library' | 'downloads') {
      this.activeTab = tab
    },

    // ── Action: Start download ──────────────────────────────────────────────
    startDownload(gameId: string) {
      const game = this.games.find(g => g.id === gameId)
      if (!game) return
      this.updateGameState(gameId, { status: 'downloading', progressPercentage: 0 })
      this.sendToNative({
        action: 'START_DOWNLOAD',
        gameId,
        payload: {
          url: game.downloadUrl,
          targetDirectory: `${this.baseInstallDir}/${game.id}/${game.executableName}`,
        },
      })
    },

    // ── Action: Pause download ──────────────────────────────────────────────
    pauseDownload(gameId: string) {
      this.updateGameState(gameId, { status: 'paused' })
      this.sendToNative({ action: 'PAUSE_DOWNLOAD', gameId, payload: {} })
    },

    // ── Action: Resume download ─────────────────────────────────────────────
    resumeDownload(gameId: string) {
      this.updateGameState(gameId, { status: 'downloading' })
      this.sendToNative({ action: 'RESUME_DOWNLOAD', gameId, payload: {} })
    },

    // ── Action: Launch game ─────────────────────────────────────────────────
    launchGame(gameId: string) {
      const game = this.games.find(g => g.id === gameId)
      if (!game) return
      this.updateGameState(gameId, { status: 'launching' })
      this.sendToNative({
        action: 'LAUNCH_GAME',
        gameId,
        payload: {
          executablePath: `${this.baseInstallDir}/${game.id}/${game.executableName}`,
          workingDirectory: `${this.baseInstallDir}/${game.id}`,
        },
      })
    },

    // ── Action: Check if installed ──────────────────────────────────────────
    checkInstalled(gameId: string) {
      const game = this.games.find(g => g.id === gameId)
      if (!game) return
      this.sendToNative({
        action: 'CHECK_INSTALLED',
        gameId,
        payload: {
          directory: `${this.baseInstallDir}/${game.id}`,
          executableName: game.executableName,
        },
      })
    },

    // ── Handle incoming native message ──────────────────────────────────────
    handleNativeMessage(msg: any) {
      const { event, gameId, data } = msg

      if (event === 'DOWNLOAD_PROGRESS_UPDATE' && gameId) {
        this.updateGameState(gameId, {
          status: data.status,
          progressPercentage: data.progressPercentage ?? 0,
          downloadSpeed: data.downloadSpeed ?? '',
          eta: data.estimatedTimeArrival ?? '',
          errorMessage: data.errorMessage,
        })
      }

      if (event === 'GAME_EXECUTION_STATUS' && gameId) {
        this.updateGameState(gameId, {
          status: data.status,
          errorMessage: data.errorMessage,
        })
      }

      if (event === 'INSTALL_STATUS' && gameId) {
        if (data.installed) {
          this.updateGameState(gameId, { status: 'installed' })
        }
      }

      if (event === 'LIBRARY_SCAN_RESULT') {
        const foundDirs: string[] = data.games ?? []
        for (const game of this.games) {
          if (foundDirs.includes(game.id)) {
            this.updateGameState(game.id, { status: 'installed' })
          }
        }
      }
    },

    // ── Internal helpers ────────────────────────────────────────────────────
    updateGameState(gameId: string, patch: Partial<GameState>) {
      const current = this.gameStates[gameId] ?? {
        id: gameId,
        status: 'idle' as GameStatus,
        progressPercentage: 0,
        downloadSpeed: '',
        eta: '',
      }
      this.gameStates[gameId] = { ...current, ...patch }
    },

    sendToNative(msg: object) {
      const str = JSON.stringify(msg)
      if (typeof (window as any).sendToNative === 'function') {
        ;(window as any).sendToNative(str)
      } else if (typeof (window as any).postToNative === 'function') {
        ;(window as any).postToNative(msg)
      } else {
        console.warn('[Bridge] sendToNative not available:', str)
      }
    },
  },
})
