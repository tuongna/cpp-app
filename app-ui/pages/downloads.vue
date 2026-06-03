<template>
  <div class="flex flex-col h-full">
    <div class="px-6 py-4 border-b border-steam-border bg-steam-dark/50 flex items-center gap-4 shrink-0">
      <h1 class="text-xl font-bold text-steam-text">Downloads</h1>
      <span class="text-steam-textDim text-xs">{{ activeDownloads.length }} active</span>
    </div>

    <div class="flex-1 overflow-y-auto p-6 space-y-3">
      <!-- Active downloads -->
      <div v-if="activeDownloads.length > 0">
        <h2 class="text-sm font-semibold text-steam-textDim uppercase tracking-wider mb-3">Active</h2>
        <DownloadItem
          v-for="game in activeDownloads"
          :key="game.id"
          :game="game"
          :state="store.getGameState(game.id)"
          @pause="store.pauseDownload(game.id)"
          @resume="store.resumeDownload(game.id)"
          @cancel="store.sendToNative({ action: 'CANCEL_DOWNLOAD', gameId: game.id, payload: {} })"
        />
      </div>

      <!-- Installed games summary -->
      <div v-if="installedGames.length > 0">
        <h2 class="text-sm font-semibold text-steam-textDim uppercase tracking-wider mb-3 mt-6">Installed</h2>
        <div class="grid grid-cols-1 gap-2">
          <div
            v-for="game in installedGames"
            :key="game.id"
            class="flex items-center gap-4 bg-steam-dark rounded p-3 border border-steam-border"
          >
            <div class="w-12 h-6 bg-steam-panel rounded overflow-hidden shrink-0">
              <img
                v-if="game.headerImage"
                :src="game.headerImage"
                :alt="game.title"
                class="w-full h-full object-cover"
              >
            </div>
            <div class="flex-1 min-w-0">
              <p class="text-sm text-steam-text font-medium truncate">{{ game.title }}</p>
              <p class="text-xs text-steam-textDim">{{ formatSize(game.sizeMB) }}</p>
            </div>
            <StatusBadge :status="store.getGameState(game.id).status" />
          </div>
        </div>
      </div>

      <!-- Empty state -->
      <div
        v-if="activeDownloads.length === 0 && installedGames.length === 0"
        class="flex flex-col items-center justify-center h-64 text-steam-textDim"
      >
        <svg class="w-16 h-16 mb-4 opacity-30" fill="none" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round"
                d="M3 16.5v2.25A2.25 2.25 0 0 0 5.25 21h13.5A2.25 2.25 0 0 0 21 18.75V16.5M16.5 12 12 16.5m0 0L7.5 12m4.5 4.5V3" />
        </svg>
        <p>No downloads yet.</p>
        <p class="text-xs mt-1 opacity-70">Install games from the Store.</p>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useGameStore } from '~/stores/gameStore'

const store = useGameStore()

const activeDownloads = computed(() => store.downloadingGames)
const installedGames  = computed(() => store.installedGames)

function formatSize(mb: number) {
  if (mb >= 1000) return `${(mb / 1024).toFixed(1)} GB`
  return `${mb} MB`
}
</script>
