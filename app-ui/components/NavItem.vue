<template>
  <button
    :class="[
      'w-full flex items-center gap-3 px-3 py-2 rounded text-sm font-medium transition-colors',
      isActive
        ? 'bg-steam-blue text-steam-darkest'
        : 'text-steam-textDim hover:bg-steam-panel hover:text-steam-text',
    ]"
    @click="navigate"
  >
    <!-- Icon -->
    <component :is="iconComponent" class="w-4 h-4 shrink-0" />

    <span class="flex-1 text-left">{{ label }}</span>

    <!-- Badge -->
    <span
      v-if="badge !== undefined"
      class="ml-auto bg-steam-blue text-steam-darkest text-xs font-bold
             min-w-[18px] h-[18px] rounded-full flex items-center justify-center px-1"
    >
      {{ badge }}
    </span>
  </button>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useGameStore } from '~/stores/gameStore'
import { useRouter, useRoute } from '#app'

const props = defineProps<{
  icon: 'store' | 'library' | 'downloads'
  label: string
  tab: 'store' | 'library' | 'downloads'
  badge?: number
}>()

const store  = useGameStore()
const router = useRouter()
const route  = useRoute()

const isActive = computed(() => route.path === `/${props.tab === 'store' ? '' : props.tab}`.replace('//', '/'))

function navigate() {
  store.setTab(props.tab)
  if (props.tab === 'store')      router.push('/')
  else if (props.tab === 'library')   router.push('/library')
  else if (props.tab === 'downloads') router.push('/downloads')
}

// Inline SVG icons as components
const iconComponent = computed(() => {
  const icons: Record<string, string> = {
    store: `<svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M13.5 21v-7.5a.75.75 0 0 1 .75-.75h3a.75.75 0 0 1 .75.75V21m-4.5 0H2.36m11.14 0H18m0 0h3.64m-1.39 0V9.349M3.75 21V9.349m0 0a3.001 3.001 0 0 0 3.75-.615A2.993 2.993 0 0 0 9.75 9.75c.896 0 1.7-.393 2.25-1.016a2.993 2.993 0 0 0 2.25 1.016c.896 0 1.7-.393 2.25-1.015a3.001 3.001 0 0 0 3.75.614m-16.5 0a3.004 3.004 0 0 1-.621-4.72l1.189-1.19A1.5 1.5 0 0 1 5.378 3h13.243a1.5 1.5 0 0 1 1.06.44l1.19 1.189a3 3 0 0 1-.621 4.72M6.75 18h3.75a.75.75 0 0 0 .75-.75V13.5a.75.75 0 0 0-.75-.75H6.75a.75.75 0 0 0-.75.75v3.75c0 .414.336.75.75.75Z" /></svg>`,
    library: `<svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M2.25 12l8.954-8.955c.44-.439 1.152-.439 1.591 0L21.75 12M4.5 9.75v10.125c0 .621.504 1.125 1.125 1.125H9.75v-4.875c0-.621.504-1.125 1.125-1.125h2.25c.621 0 1.125.504 1.125 1.125V21h4.125c.621 0 1.125-.504 1.125-1.125V9.75M8.25 21h8.25" /></svg>`,
    downloads: `<svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M3 16.5v2.25A2.25 2.25 0 0 0 5.25 21h13.5A2.25 2.25 0 0 0 21 18.75V16.5M16.5 12 12 16.5m0 0L7.5 12m4.5 4.5V3" /></svg>`,
  }
  const svgStr = icons[props.icon]
  return { template: svgStr }
})
</script>
