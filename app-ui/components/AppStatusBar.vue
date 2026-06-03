<template>
  <footer class="h-8 bg-steam-darker border-t border-steam-border flex items-center px-4 gap-6 shrink-0">
    <!-- Active downloads summary -->
    <div v-if="activeDownload" class="flex items-center gap-3 flex-1">
      <div class="w-24 h-1.5 bg-steam-darkest rounded-full overflow-hidden">
        <div
          class="h-full bg-steam-blue rounded-full transition-all"
          :style="{ width: `${activeDownload.state.progressPercentage}%` }"
        />
      </div>
      <span class="text-steam-textDim text-xs">
        {{ activeDownload.game.title }} —
        {{ activeDownload.state.progressPercentage.toFixed(1) }}%
        <span v-if="activeDownload.state.downloadSpeed" class="text-steam-blue ml-1">
          {{ activeDownload.state.downloadSpeed }}
        </span>
      </span>
    </div>
    <div v-else class="flex-1">
      <span class="text-steam-textDim text-xs">Ready</span>
    </div>

    <!-- Currently playing -->
    <div v-if="playingGame" class="flex items-center gap-2">
      <span class="w-2 h-2 rounded-full bg-green-400 animate-pulse" />
      <span class="text-green-400 text-xs font-medium">{{ playingGame.title }}</span>
    </div>

    <!-- Clock -->
    <span class="text-steam-textDim text-xs tabular-nums shrink-0">{{ clock }}</span>
  </footer>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useGameStore } from '~/stores/gameStore'

const store = useGameStore()
const clock = ref('')

const activeDownload = computed(() => {
  const dl = store.downloadingGames[0]
  if (!dl) return null
  return { game: dl, state: store.getGameState(dl.id) }
})

const playingGame = computed(() =>
  store.games.find(g => store.getGameState(g.id).status === 'playing')
)

let timer: ReturnType<typeof setInterval>

function updateClock() {
  clock.value = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
}

onMounted(() => {
  updateClock()
  timer = setInterval(updateClock, 10_000)
})
onUnmounted(() => clearInterval(timer))
</script>
