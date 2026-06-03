<template>
  <div
    class="group relative rounded overflow-hidden cursor-pointer bg-steam-dark
           border border-transparent hover:border-steam-blue transition-all
           hover:shadow-lg hover:shadow-steam-blue/20"
    @click="$emit('click', game.id)"
  >
    <!-- Game header image -->
    <div class="relative aspect-[460/215] overflow-hidden bg-steam-panel">
      <img
        v-if="game.headerImage"
        :src="game.headerImage"
        :alt="game.title"
        class="w-full h-full object-cover transition-transform duration-300 group-hover:scale-105"
        loading="lazy"
        @error="imageError = true"
      >
      <div
        v-if="!game.headerImage || imageError"
        class="w-full h-full flex items-center justify-center text-steam-textDim text-sm"
      >
        {{ game.title }}
      </div>

      <!-- Status overlay badge -->
      <div class="absolute top-2 right-2">
        <StatusBadge :status="state.status" />
      </div>

      <!-- Playing overlay -->
      <div
        v-if="state.status === 'playing'"
        class="absolute inset-0 bg-steam-blue/10 flex items-center justify-center"
      >
        <div class="bg-steam-blue/90 text-steam-darkest text-xs font-bold px-3 py-1 rounded-full
                    flex items-center gap-1.5 animate-pulse">
          <span class="w-1.5 h-1.5 rounded-full bg-steam-darkest" />
          PLAYING
        </div>
      </div>
    </div>

    <!-- Game info -->
    <div class="p-3">
      <h3 class="font-semibold text-steam-text text-sm truncate group-hover:text-steam-blue transition-colors">
        {{ game.title }}
      </h3>
      <p class="text-steam-textDim text-xs mt-0.5 truncate">{{ game.developer }}</p>

      <!-- Download progress bar (shown when downloading) -->
      <DownloadBar
        v-if="state.status === 'downloading' || state.status === 'paused'"
        :progress="state.progressPercentage"
        :speed="state.downloadSpeed"
        :eta="state.eta"
        :paused="state.status === 'paused'"
        class="mt-2"
      />

      <!-- Genre tags -->
      <div v-else class="flex gap-1 mt-2 flex-wrap">
        <span
          v-for="genre in game.genres.slice(0, 3)"
          :key="genre"
          class="text-xs bg-steam-panel text-steam-textDim px-1.5 py-0.5 rounded"
        >
          {{ genre }}
        </span>
      </div>

      <!-- Action button -->
      <div class="mt-3">
        <ActionButton :game="game" :state="state" @action="handleAction" />
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

const emit = defineEmits<{
  click: [id: string]
  action: [type: string, gameId: string]
}>()

const imageError = ref(false)

function handleAction(type: string, gameId: string) {
  emit('action', type, gameId)
}
</script>
