<template>
  <aside class="flex flex-col bg-steam-dark overflow-y-auto">
    <!-- Close button -->
    <div class="flex items-center justify-between px-4 py-3 border-b border-steam-border shrink-0">
      <span class="text-sm font-semibold text-steam-textDim truncate pr-2">Game Details</span>
      <button
        class="text-steam-textDim hover:text-steam-text p-1 rounded hover:bg-steam-panel transition-colors"
        @click="$emit('close')"
      >
        <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" d="M6 18 18 6M6 6l12 12" />
        </svg>
      </button>
    </div>

    <!-- Header image -->
    <div class="relative aspect-[460/215] bg-steam-panel shrink-0">
      <img
        v-if="game.headerImage"
        :src="game.headerImage"
        :alt="game.title"
        class="w-full h-full object-cover"
        @error="imgError = true"
      >
      <div v-if="imgError || !game.headerImage"
           class="absolute inset-0 flex items-center justify-center text-steam-textDim">
        {{ game.title }}
      </div>
      <div class="absolute inset-0 bg-gradient-to-t from-steam-dark to-transparent" />
    </div>

    <!-- Game info -->
    <div class="p-4 flex-1 flex flex-col gap-4">
      <div>
        <h2 class="text-lg font-bold text-steam-text">{{ game.title }}</h2>
        <p class="text-steam-textDim text-xs mt-0.5">{{ game.developer }} · {{ game.releaseYear }}</p>
      </div>

      <!-- Rating -->
      <div class="flex items-center gap-2">
        <div class="flex-1 h-2 bg-steam-darkest rounded-full overflow-hidden">
          <div
            class="h-full rounded-full transition-all"
            :class="ratingColor"
            :style="{ width: `${game.rating}%` }"
          />
        </div>
        <span class="text-sm font-bold" :class="ratingTextColor">{{ game.rating }}%</span>
      </div>

      <!-- Description -->
      <p class="text-steam-textDim text-xs leading-relaxed">{{ game.description }}</p>

      <!-- Metadata -->
      <dl class="grid grid-cols-2 gap-x-4 gap-y-1 text-xs">
        <dt class="text-steam-textDim">Size</dt>
        <dd class="text-steam-text">{{ formatSize(game.sizeMB) }}</dd>
        <dt class="text-steam-textDim">Publisher</dt>
        <dd class="text-steam-text truncate">{{ game.publisher }}</dd>
        <dt class="text-steam-textDim">Status</dt>
        <dd><StatusBadge :status="state.status" /></dd>
      </dl>

      <!-- Genres -->
      <div class="flex flex-wrap gap-1">
        <span
          v-for="genre in game.genres"
          :key="genre"
          class="text-xs bg-steam-panel text-steam-textDim px-2 py-0.5 rounded"
        >
          {{ genre }}
        </span>
      </div>

      <!-- Download progress (when active) -->
      <div v-if="state.status === 'downloading' || state.status === 'paused'" class="space-y-1.5">
        <div class="flex justify-between text-xs text-steam-textDim">
          <span>Downloading...</span>
          <span class="text-steam-blue font-medium">{{ state.downloadSpeed }}</span>
        </div>
        <DownloadBar
          :progress="state.progressPercentage"
          :speed="state.downloadSpeed"
          :eta="state.eta"
          :paused="state.status === 'paused'"
        />
        <div class="text-right text-xs text-steam-textDim">ETA {{ state.eta }}</div>
      </div>

      <!-- Action buttons -->
      <div class="mt-auto">
        <ActionButton :game="game" :state="state" @action="(t, id) => $emit('action', t, id)" />
      </div>
    </div>
  </aside>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import type { Game, GameState } from '~/stores/gameStore'

const props = defineProps<{
  game: Game
  state: GameState
}>()

defineEmits<{
  action: [type: string, gameId: string]
  close: []
}>()

const imgError = ref(false)

const ratingColor = computed(() => {
  if (props.game.rating >= 80) return 'bg-green-500'
  if (props.game.rating >= 60) return 'bg-yellow-500'
  return 'bg-red-500'
})

const ratingTextColor = computed(() => {
  if (props.game.rating >= 80) return 'text-green-400'
  if (props.game.rating >= 60) return 'text-yellow-400'
  return 'text-red-400'
})

function formatSize(mb: number): string {
  if (mb >= 1000) return `${(mb / 1024).toFixed(1)} GB`
  return `${mb} MB`
}
</script>
