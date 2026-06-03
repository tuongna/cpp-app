<template>
  <div class="flex flex-col h-full">
    <!-- Header -->
    <div class="px-6 py-4 border-b border-steam-border bg-steam-dark/50 flex items-center gap-4 shrink-0">
      <h1 class="text-xl font-bold text-steam-text">Library</h1>
      <span class="text-steam-textDim text-xs">{{ installedGames.length }} games installed</span>

      <button
        class="ml-auto text-xs text-steam-textDim hover:text-steam-blue flex items-center gap-1 transition-colors"
        @click="scanLibrary"
      >
        <svg class="w-3.5 h-3.5" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round"
                d="M16.023 9.348h4.992v-.001M2.985 19.644v-4.992m0 0h4.992m-4.993 0 3.181 3.183a8.25 8.25 0 0 0 13.803-3.7M4.031 9.865a8.25 8.25 0 0 1 13.803-3.7l3.181 3.182m0-4.991v4.99" />
        </svg>
        Scan Library
      </button>
    </div>

    <!-- Content -->
    <div class="flex flex-1 overflow-hidden">
      <!-- Game list (left column) -->
      <div class="w-64 border-r border-steam-border overflow-y-auto shrink-0 bg-steam-darker/30">
        <div v-if="installedGames.length === 0" class="p-6 text-center text-steam-textDim text-sm">
          <svg class="w-12 h-12 mx-auto mb-3 opacity-30" fill="none" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round"
                  d="M2.25 12.75V12A2.25 2.25 0 0 1 4.5 9.75h15A2.25 2.25 0 0 1 21.75 12v.75m-8.69-6.44-2.12-2.12a1.5 1.5 0 0 0-1.061-.44H4.5A2.25 2.25 0 0 0 2.25 6v12a2.25 2.25 0 0 0 2.25 2.25h15A2.25 2.25 0 0 0 21.75 18V9a2.25 2.25 0 0 0-2.25-2.25h-5.379a1.5 1.5 0 0 1-1.06-.44Z" />
          </svg>
          <p>No games installed.</p>
          <p class="mt-1 text-xs opacity-70">Visit the Store to install games.</p>
        </div>

        <button
          v-for="game in installedGames"
          :key="game.id"
          :class="[
            'w-full flex items-center gap-3 px-4 py-3 border-b border-steam-border/50 transition-colors',
            store.selectedGameId === game.id
              ? 'bg-steam-blue/20 border-l-2 border-l-steam-blue'
              : 'hover:bg-steam-panel/50',
          ]"
          @click="store.selectGame(game.id)"
        >
          <div class="w-8 h-8 rounded bg-steam-panel flex items-center justify-center shrink-0 overflow-hidden">
            <img
              v-if="game.headerImage"
              :src="game.headerImage"
              :alt="game.title"
              class="w-full h-full object-cover"
              @error="($event.target as HTMLElement).style.display='none'"
            >
          </div>
          <div class="min-w-0 text-left">
            <p class="text-sm text-steam-text truncate">{{ game.title }}</p>
            <div class="flex items-center gap-1 mt-0.5">
              <span
                v-if="getState(game.id).status === 'playing'"
                class="w-1.5 h-1.5 rounded-full bg-green-400 animate-pulse"
              />
              <p class="text-xs text-steam-textDim">{{ game.developer }}</p>
            </div>
          </div>
        </button>
      </div>

      <!-- Game detail (right panel) -->
      <div v-if="selectedInstalledGame" class="flex-1 overflow-y-auto">
        <LibraryGameView
          :game="selectedInstalledGame"
          :state="store.getGameState(selectedInstalledGame.id)"
          @action="handleAction"
        />
      </div>

      <div v-else class="flex-1 flex items-center justify-center text-steam-textDim">
        <p>Select a game from your library</p>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useGameStore } from '~/stores/gameStore'

const store = useGameStore()

const installedGames = computed(() => store.installedGames)

const selectedInstalledGame = computed(() =>
  installedGames.value.find(g => g.id === store.selectedGameId) ?? null
)

function getState(id: string) {
  return store.getGameState(id)
}

function scanLibrary() {
  store.sendToNative({
    action: 'SCAN_LIBRARY',
    gameId: '',
    payload: { baseDirectory: store.baseInstallDir },
  })
}

function handleAction(type: string, gameId: string) {
  switch (type) {
    case 'play':   store.launchGame(gameId); break
    case 'pause':  store.pauseDownload(gameId); break
    case 'resume': store.resumeDownload(gameId); break
  }
}
</script>
