import type { Config } from 'tailwindcss'

export default {
  content: [
    './components/**/*.{vue,js,ts}',
    './layouts/**/*.vue',
    './pages/**/*.vue',
    './app.vue',
  ],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        // Steam-inspired dark theme palette
        steam: {
          dark:     '#1b2838',
          darker:   '#171a21',
          darkest:  '#0e141b',
          panel:    '#2a3f5f',
          blue:     '#66c0f4',
          blueDark: '#1b9cdb',
          green:    '#5ba32b',
          greenHov: '#4c8f24',
          text:     '#c7d5e0',
          textDim:  '#8f98a0',
          border:   '#2e4057',
          red:      '#d94e3c',
          gold:     '#c6a84b',
          accent:   '#375a7f',
        },
      },
      fontFamily: {
        steam: ['"Motiva Sans"', '"Arial"', 'sans-serif'],
      },
      animation: {
        'progress-pulse': 'pulse 1.5s cubic-bezier(0.4, 0, 0.6, 1) infinite',
      },
    },
  },
  plugins: [],
} satisfies Config
