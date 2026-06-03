<template>
  <div class="flex h-full">
    <!-- Game list panel -->
    <div class="flex flex-col flex-1 overflow-hidden">
      <!-- Store header -->
      <div class="px-6 py-4 border-b border-steam-border bg-steam-dark/50 flex items-center gap-4 shrink-0">
        <h1 class="text-xl font-bold text-steam-text">Store</h1>

        <!-- Search bar -->
        <div class="relative flex-1 max-w-xs">
          <input
            v-model="searchQuery"
            type="text"
            placeholder="Search games..."
            class="w-full bg-steam-darkest border border-steam-border rounded px-3 py-1.5 text-sm
                   text-steam-text placeholder:text-steam-textDim
                   focus:outline-none focus:border-steam-blue transition-colors"
          >
          <svg class="absolute right-2.5 top-1/2 -translate-y-1/2 w-4 h-4 text-steam-textDim pointer-events-none"
               fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round"
                  d="m21 21-5.197-5.197m0 0A7.5 7.5 0 1 0 5.196 5.196a7.5 7.5 0 0 0 10.607 10.607Z" />
          </svg>
        </div>

        <!-- Genre filter -->
        <select
          v-model="selectedGenre"
          class="bg-steam-darkest border border-steam-border rounded px-2 py-1.5 text-sm
                 text-steam-text focus:outline-none focus:border-steam-blue"
        >
          <option value="">All Genres</option>
          <option v-for="genre in allGenres" :key="genre" :value="genre">{{ genre }}</option>
        </select>

        <span class="text-steam-textDim text-xs ml-auto">{{ filteredGames.length }} games</span>
      </div>

      <!-- Game grid -->
      <div class="flex-1 overflow-y-auto p-6">
        <div class="grid grid-cols-2 xl:grid-cols-3 2xl:grid-cols-4 gap-4">
          <GameCard
            v-for="game in filteredGames"
            :key="game.id"
            :game="game"
            :state="store.getGameState(game.id)"
            @click="selectGame"
            @action="handleAction"
          />
        </div>

        <div v-if="filteredGames.length === 0" class="flex items-center justify-center h-48 text-steam-textDim">
          No games match your search.
        </div>
      </div>
    </div>

    <!-- Game detail panel (sidebar) -->
    <Transition name="slide">
      <GameDetail
        v-if="store.selectedGame"
        :game="store.selectedGame"
        :state="store.getGameState(store.selectedGame.id)"
        class="w-80 border-l border-steam-border"
        @action="handleAction"
        @close="store.selectGame('')"
      />
    </Transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useGameStore } from '~/stores/gameStore'

const store = useGameStore()
const searchQuery  = ref('')
const selectedGenre = ref('')

const allGenres = computed(() => {
  const set = new Set<string>()
  for (const g of store.games)
    for (const genre of g.genres)
      set.add(genre)
  return [...set].sort()
})

const filteredGames = computed(() => {
  let list = store.games
  if (searchQuery.value.trim())
    list = list.filter(g =>
      g.title.toLowerCase().includes(searchQuery.value.toLowerCase()) ||
      g.developer.toLowerCase().includes(searchQuery.value.toLowerCase())
    )
  if (selectedGenre.value)
    list = list.filter(g => g.genres.includes(selectedGenre.value))
  return list
})

function selectGame(id: string) {
  store.selectGame(id === store.selectedGameId ? '' : id)
}

function handleAction(type: string, gameId: string) {
  switch (type) {
    case 'install':  store.startDownload(gameId); break
    case 'play':     store.launchGame(gameId);    break
    case 'pause':    store.pauseDownload(gameId); break
    case 'resume':   store.resumeDownload(gameId); break
    case 'cancel':   store.sendToNative({ action: 'CANCEL_DOWNLOAD', gameId, payload: {} }); break
    case 'retry':    store.startDownload(gameId); break
  }
}
</script>

<style scoped>
.slide-enter-active,
.slide-leave-active { transition: transform 0.25s ease, opacity 0.25s ease; }
.slide-enter-from,
.slide-leave-to    { transform: translateX(100%); opacity: 0; }
</style>
