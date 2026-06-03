<template>
  <aside class="w-52 bg-steam-darker flex flex-col shrink-0 border-r border-steam-border">
    <!-- User avatar section -->
    <div class="p-4 border-b border-steam-border flex items-center gap-3">
      <div class="w-10 h-10 rounded bg-steam-panel flex items-center justify-center shrink-0">
        <svg class="w-6 h-6 text-steam-blue" fill="currentColor" viewBox="0 0 24 24">
          <path d="M12 12c2.7 0 4.8-2.1 4.8-4.8S14.7 2.4 12 2.4 7.2 4.5 7.2 7.2 9.3 12 12 12zm0 2.4c-3.2 0-9.6 1.6-9.6 4.8v2.4h19.2v-2.4c0-3.2-6.4-4.8-9.6-4.8z"/>
        </svg>
      </div>
      <div class="min-w-0">
        <p class="text-sm font-semibold text-steam-text truncate">Player One</p>
        <p class="text-xs text-steam-textDim">Online</p>
      </div>
    </div>

    <!-- Navigation -->
    <nav class="flex-1 p-2 space-y-0.5">
      <p class="text-steam-textDim text-xs font-semibold uppercase tracking-widest px-3 py-2 mt-1">
        Navigation
      </p>

      <NavItem
        icon="store"
        label="Store"
        tab="store"
      />
      <NavItem
        icon="library"
        label="Library"
        tab="library"
      />
      <NavItem
        icon="downloads"
        label="Downloads"
        tab="downloads"
        :badge="downloadingCount > 0 ? downloadingCount : undefined"
      />

      <div class="border-t border-steam-border my-2" />

      <p class="text-steam-textDim text-xs font-semibold uppercase tracking-widest px-3 py-2 mt-1">
        Recent
      </p>

      <button
        v-for="game in recentGames"
        :key="game.id"
        class="w-full flex items-center gap-2 px-3 py-2 rounded text-sm text-steam-textDim
               hover:bg-steam-panel hover:text-steam-text transition-colors"
        @click="selectGame(game.id)"
      >
        <span class="truncate">{{ game.title }}</span>
        <!-- Playing indicator -->
        <span
          v-if="getState(game.id).status === 'playing'"
          class="ml-auto w-2 h-2 rounded-full bg-green-400 shrink-0 animate-pulse"
        />
      </button>
    </nav>

    <!-- Bottom: disk usage -->
    <div class="p-3 border-t border-steam-border">
      <p class="text-steam-textDim text-xs mb-1">Install Drive</p>
      <div class="h-1.5 bg-steam-darkest rounded-full overflow-hidden">
        <div class="h-full bg-steam-blue rounded-full" style="width: 38%" />
      </div>
      <p class="text-steam-textDim text-xs mt-1">238 GB free</p>
    </div>
  </aside>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useGameStore } from '~/stores/gameStore'
import { useRouter } from '#app'

const store  = useGameStore()
const router = useRouter()

const downloadingCount = computed(() => store.downloadingGames.length)

const recentGames = computed(() =>
  store.installedGames.slice(0, 5)
)

function getState(id: string) {
  return store.getGameState(id)
}

function selectGame(id: string) {
  store.selectGame(id)
  store.setTab('library')
  router.push('/library')
}
</script>
