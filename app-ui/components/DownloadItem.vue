<template>
  <div class="bg-steam-dark rounded border border-steam-border p-4 flex gap-4 items-start">
    <!-- Thumbnail -->
    <div class="w-20 h-10 bg-steam-panel rounded overflow-hidden shrink-0">
      <img
        v-if="game.headerImage"
        :src="game.headerImage"
        :alt="game.title"
        class="w-full h-full object-cover"
      >
    </div>

    <!-- Info -->
    <div class="flex-1 min-w-0 space-y-2">
      <div class="flex items-center gap-2">
        <p class="text-sm font-semibold text-steam-text truncate">{{ game.title }}</p>
        <StatusBadge :status="state.status" />
      </div>

      <DownloadBar
        :progress="state.progressPercentage"
        :speed="state.downloadSpeed"
        :eta="state.eta"
        :paused="state.status === 'paused'"
      />

      <div class="flex items-center gap-1 text-xs text-steam-textDim">
        <span>{{ formatSize(game.sizeMB) }} total</span>
        <span v-if="state.eta && state.status === 'downloading'"> · ETA {{ state.eta }}</span>
      </div>
    </div>

    <!-- Controls -->
    <div class="flex items-center gap-2 shrink-0">
      <button
        v-if="state.status === 'downloading'"
        class="p-1.5 rounded bg-steam-panel hover:bg-steam-accent text-steam-textDim hover:text-steam-text transition-colors"
        title="Pause"
        @click="$emit('pause')"
      >
        <svg class="w-4 h-4" fill="currentColor" viewBox="0 0 24 24">
          <path d="M6 19h4V5H6v14zm8-14v14h4V5h-4z"/>
        </svg>
      </button>

      <button
        v-if="state.status === 'paused'"
        class="p-1.5 rounded bg-steam-panel hover:bg-steam-blue hover:text-steam-darkest text-steam-textDim transition-colors"
        title="Resume"
        @click="$emit('resume')"
      >
        <svg class="w-4 h-4" fill="currentColor" viewBox="0 0 24 24">
          <path d="M8 5v14l11-7z"/>
        </svg>
      </button>

      <button
        class="p-1.5 rounded bg-steam-panel hover:bg-red-700 text-steam-textDim hover:text-white transition-colors"
        title="Cancel"
        @click="$emit('cancel')"
      >
        <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" d="M6 18 18 6M6 6l12 12" />
        </svg>
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import type { Game, GameState } from '~/stores/gameStore'

defineProps<{
  game: Game
  state: GameState
}>()

defineEmits<{
  pause: []
  resume: []
  cancel: []
}>()

function formatSize(mb: number) {
  if (mb >= 1000) return `${(mb / 1024).toFixed(1)} GB`
  return `${mb} MB`
}
</script>
