<template>
  <div class="flex gap-2">
    <!-- Primary action -->
    <button
      :class="[
        'flex-1 py-1.5 px-3 rounded text-sm font-semibold transition-all flex items-center justify-center gap-1.5',
        primaryClass,
        disabled ? 'opacity-50 cursor-not-allowed' : 'cursor-pointer',
      ]"
      :disabled="disabled"
      @click.stop="emit('action', primaryAction, game.id)"
    >
      <span>{{ primaryLabel }}</span>
    </button>

    <!-- Secondary action (pause/resume) -->
    <button
      v-if="secondaryAction"
      class="py-1.5 px-2.5 rounded text-sm font-semibold bg-steam-panel hover:bg-steam-accent
             text-steam-textDim hover:text-steam-text transition-all"
      @click.stop="emit('action', secondaryAction, game.id)"
    >
      {{ secondaryLabel }}
    </button>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { Game, GameState } from '~/stores/gameStore'

const props = defineProps<{
  game: Game
  state: GameState
}>()

const emit = defineEmits<{
  action: [type: string, gameId: string]
}>()

const primaryAction = computed(() => {
  switch (props.state.status) {
    case 'idle':        return 'install'
    case 'installed':   return 'play'
    case 'downloading': return 'cancel'
    case 'paused':      return 'resume'
    case 'playing':     return 'playing'
    case 'launching':   return 'launching'
    case 'error':       return 'retry'
    default:            return 'install'
  }
})

const primaryLabel = computed(() => {
  switch (props.state.status) {
    case 'idle':        return 'Install'
    case 'installed':   return '▶ Play'
    case 'downloading': return 'Cancel'
    case 'paused':      return '▶ Resume'
    case 'playing':     return 'Playing...'
    case 'launching':   return 'Launching...'
    case 'error':       return 'Retry'
    default:            return 'Install'
  }
})

const primaryClass = computed(() => {
  switch (props.state.status) {
    case 'idle':
    case 'error':
      return 'bg-steam-green hover:bg-steam-greenHov text-white'
    case 'installed':
      return 'bg-steam-blue hover:bg-steam-blueDark text-steam-darkest'
    case 'downloading':
      return 'bg-red-700 hover:bg-red-600 text-white'
    case 'paused':
      return 'bg-yellow-600 hover:bg-yellow-500 text-white'
    case 'playing':
    case 'launching':
      return 'bg-steam-panel text-steam-textDim'
    default:
      return 'bg-steam-green hover:bg-steam-greenHov text-white'
  }
})

const disabled = computed(() =>
  props.state.status === 'playing' || props.state.status === 'launching'
)

const secondaryAction = computed(() => {
  if (props.state.status === 'downloading') return 'pause'
  return null
})

const secondaryLabel = computed(() => {
  if (props.state.status === 'downloading') return '⏸'
  return ''
})
</script>
