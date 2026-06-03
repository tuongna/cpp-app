<template>
  <span
    v-if="label"
    :class="['text-xs font-bold px-2 py-0.5 rounded', colorClass]"
  >
    {{ label }}
  </span>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { GameStatus } from '~/stores/gameStore'

const props = defineProps<{ status: GameStatus }>()

const label = computed(() => {
  switch (props.status) {
    case 'installed':    return 'Installed'
    case 'downloading':  return 'Downloading'
    case 'paused':       return 'Paused'
    case 'playing':      return 'Playing'
    case 'launching':    return 'Launching'
    case 'error':        return 'Error'
    default:             return ''
  }
})

const colorClass = computed(() => {
  switch (props.status) {
    case 'installed':    return 'bg-green-700/80 text-green-200'
    case 'downloading':  return 'bg-steam-blue/80 text-steam-darkest'
    case 'paused':       return 'bg-yellow-600/80 text-yellow-100'
    case 'playing':      return 'bg-green-500/80 text-white'
    case 'launching':    return 'bg-steam-blue/60 text-white'
    case 'error':        return 'bg-red-700/80 text-red-100'
    default:             return ''
  }
})
</script>
