<template>
  <div class="space-y-1">
    <!-- Progress bar -->
    <div class="h-1.5 bg-steam-darkest rounded-full overflow-hidden">
      <div
        :class="[
          'h-full rounded-full transition-all duration-500',
          paused ? 'bg-yellow-500' : 'bg-steam-blue',
        ]"
        :style="{ width: `${Math.max(0, Math.min(100, progress))}%` }"
      />
    </div>

    <!-- Speed / ETA / percentage row -->
    <div class="flex justify-between text-xs text-steam-textDim">
      <span>{{ progress.toFixed(1) }}%</span>
      <span v-if="!paused && speed" class="text-steam-blue">{{ speed }}</span>
      <span v-if="paused" class="text-yellow-400">Paused</span>
      <span v-if="eta && !paused">{{ eta }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  progress: number
  speed?: string
  eta?: string
  paused?: boolean
}>()
</script>
