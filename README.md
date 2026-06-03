# Steam Clone

A cross-platform desktop game launcher: a **C++17 native shell** hosting a
**Nuxt 3 / Vue 3** web UI inside a system WebView. The native layer handles
windowing, downloads, game launching and the filesystem; the web layer handles
the entire interface. They talk over a small JSON bridge.

For the full design, see **[ARCHITECTURE.md](./ARCHITECTURE.md)**.

## Project layout

```
app-core/   C++17 native shell (WebView, downloader, launcher, JSON bridge)
app-ui/     Nuxt 3 / Vue 3 frontend (static SPA)
build.sh    Builds the UI then the C++ binary
```

## Prerequisites

- **Node.js** 18+ and npm
- **CMake** 3.16+ and a **C++17** compiler
- Platform WebView dependencies:
  - **macOS:** Xcode Command Line Tools (`xcode-select --install`)
  - **Linux:** `sudo apt-get install cmake g++ libgtk-3-dev libwebkit2gtk-4.1-dev`
  - **Windows:** Visual Studio 2019+ (C++ workload) and the WebView2 Runtime

`cpp-httplib` and `webview` are fetched automatically by CMake — no manual
vendoring needed.

## Build & run

```bash
./build.sh            # Release by default; pass Debug for a debug build
./build/SteamClone    # macOS: open ./build/SteamClone.app
```

`build.sh`:
1. installs UI dependencies (first run) and runs `npm run generate`;
2. configures and builds `app-core` with CMake;
3. copies the generated UI to `build/ui/` next to the binary.

## Develop the UI standalone

```bash
cd app-ui
npm install
npm run dev      # hot-reload dev server in a normal browser
```

When run in a plain browser the native bridge is absent; `sendToNative` logs a
warning instead of failing, so the UI still renders for development.

## How it works (in brief)

- The binary serves the built UI from a local `127.0.0.1` HTTP server and points
  the WebView at it.
- UI → native: `window.sendToNative(jsonString)` (e.g. `START_DOWNLOAD`,
  `LAUNCH_GAME`).
- Native → UI: the backend evaluates `window.onNativeMessage(event)` to push
  download progress and process status.

See [ARCHITECTURE.md §4](./ARCHITECTURE.md#4-the-bridge-ipc-protocol) for the
full message protocol.
