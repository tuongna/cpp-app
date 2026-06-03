<template>
  <div class="dark h-screen w-screen overflow-hidden bg-steam-darkest text-steam-text flex flex-col select-none">
    <!-- Title bar (custom drag area) -->
    <div class="h-8 bg-steam-darker flex items-center px-4 shrink-0 border-b border-steam-border"
         style="-webkit-app-region: drag">
      <span class="text-steam-blue font-bold text-sm tracking-wide" style="-webkit-app-region: no-drag">
        STEAM CLONE
      </span>
      <span class="ml-3 text-steam-textDim text-xs">Game Launcher</span>
    </div>

    <!-- Main layout -->
    <div class="flex flex-1 overflow-hidden">
      <!-- Sidebar navigation -->
      <AppSidebar />

      <!-- Page content -->
      <main class="flex-1 overflow-y-auto">
        <NuxtPage />
      </main>
    </div>

    <!-- Bottom status bar -->
    <AppStatusBar />
  </div>
</template>

<script setup lang="ts">
import { onMounted, onUnmounted } from 'vue'
import { useGameStore } from '~/stores/gameStore'
import { GAME_CATALOG } from '~/data/catalog'

const store = useGameStore()

// Native message handler
function onNativeMessage(msg: any) {
  store.handleNativeMessage(msg)
}

onMounted(() => {
  // Initialize game catalog
  store.initGames(GAME_CATALOG)

  // Register global native message receiver
  ;(window as any).onNativeMessage = onNativeMessage

  // Scan library on startup
  store.sendToNative({
    action: 'SCAN_LIBRARY',
    gameId: '',
    payload: { baseDirectory: store.baseInstallDir },
  })
})

onUnmounted(() => {
  ;(window as any).onNativeMessage = undefined
})
</script>
