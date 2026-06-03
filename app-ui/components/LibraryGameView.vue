<template>
  <div class="relative">
    <!-- Hero background -->
    <div class="relative h-56 overflow-hidden">
      <img
        v-if="game.backgroundImage"
        :src="game.backgroundImage"
        :alt="game.title"
        class="w-full h-full object-cover"
        @error="bgError = true"
      >
      <div class="absolute inset-0 bg-gradient-to-t from-steam-dark via-steam-dark/60 to-transparent" />

      <!-- Hero content -->
      <div class="absolute bottom-0 left-0 p-6 flex items-end gap-5">
        <div class="w-20 h-20 rounded-lg bg-steam-panel overflow-hidden border-2 border-steam-border shrink-0">
          <img
            v-if="game.headerImage"
            :src="game.headerImage"
            :alt="game.title"
            class="w-full h-full object-cover"
          >
        </div>
        <div>
          <h1 class="text-2xl font-bold text-white drop-shadow">{{ game.title }}</h1>
          <p class="text-steam-textDim text-sm mt-0.5">{{ game.developer }} · {{ game.releaseYear }}</p>
        </div>
      </div>
    </div>

    <!-- Content -->
    <div class="p-6 space-y-6">
      <!-- Status + action -->
      <div class="flex items-center gap-4">
        <div class="flex-1">
          <div v-if="state.status === 'playing'" class="flex items-center gap-2">
            <span class="w-2.5 h-2.5 rounded-full bg-green-400 animate-pulse" />
            <span class="text-green-400 font-semibold">Currently Playing</span>
          </div>
          <StatusBadge v-else :status="state.status" />
        </div>
        <ActionButton :game="game" :state="state" @action="(t,id)=>$emit('action',t,id)" />
      </div>

      <!-- Description -->
      <div>
        <h2 class="text-sm font-semibold text-steam-textDim uppercase tracking-wider mb-2">About</h2>
        <p class="text-steam-text text-sm leading-relaxed">{{ game.description }}</p>
      </div>

      <!-- Details grid -->
      <div class="grid grid-cols-2 gap-4">
        <div class="bg-steam-darker/50 rounded p-3">
          <p class="text-steam-textDim text-xs uppercase tracking-wide mb-1">Rating</p>
          <div class="flex items-center gap-2">
            <div class="flex-1 h-2 bg-steam-darkest rounded-full overflow-hidden">
              <div class="h-full bg-green-500 rounded-full" :style="{ width: `${game.rating}%` }" />
            </div>
            <span class="text-green-400 text-sm font-bold">{{ game.rating }}%</span>
          </div>
        </div>
        <div class="bg-steam-darker/50 rounded p-3">
          <p class="text-steam-textDim text-xs uppercase tracking-wide mb-1">Size</p>
          <p class="text-steam-text font-semibold">{{ formatSize(game.sizeMB) }}</p>
        </div>
        <div class="bg-steam-darker/50 rounded p-3">
          <p class="text-steam-textDim text-xs uppercase tracking-wide mb-1">Publisher</p>
          <p class="text-steam-text font-semibold truncate">{{ game.publisher }}</p>
        </div>
        <div class="bg-steam-darker/50 rounded p-3">
          <p class="text-steam-textDim text-xs uppercase tracking-wide mb-1">Year</p>
          <p class="text-steam-text font-semibold">{{ game.releaseYear }}</p>
        </div>
      </div>

      <!-- Genres -->
      <div>
        <h2 class="text-sm font-semibold text-steam-textDim uppercase tracking-wider mb-2">Genres</h2>
        <div class="flex flex-wrap gap-2">
          <span
            v-for="genre in game.genres"
            :key="genre"
            class="text-sm bg-steam-panel text-steam-textDim px-3 py-1 rounded"
          >
            {{ genre }}
          </span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import type { Game, GameState } from '~/stores/gameStore'

const props = defineProps<{
  game: Game
  state: GameState
}>()

defineEmits<{
  action: [type: string, gameId: string]
}>()

const bgError = ref(false)

function formatSize(mb: number): string {
  if (mb >= 1000) return `${(mb / 1024).toFixed(1)} GB`
  return `${mb} MB`
}
</script>
