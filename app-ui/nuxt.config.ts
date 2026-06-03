// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
  // Static Site Generation for embedding into C++ binary
  ssr: false,

  modules: [
    '@nuxtjs/tailwindcss',
    '@pinia/nuxt',
  ],

  app: {
    // Use relative base so file:// protocol works correctly
    baseURL: './',
    buildAssetsDir: '_assets',
    head: {
      title: 'Steam Clone',
      meta: [
        { charset: 'utf-8' },
        { name: 'viewport', content: 'width=device-width, initial-scale=1' },
      ],
      link: [
        { rel: 'icon', type: 'image/svg+xml', href: './favicon.svg' },
      ],
    },
  },

  nitro: {
    preset: 'static',
    output: {
      publicDir: '.output/public',
    },
  },

  // Disable server-side features not needed for SSG
  experimental: {
    payloadExtraction: false,
  },

  tailwindcss: {
    configPath: 'tailwind.config.ts',
  },

  devtools: { enabled: false },
})
