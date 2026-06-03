#include "webview.h"
#include "nlohmann/json.hpp"
#include "Downloader.h"
#include "Launcher.h"

#include <string>
#include <memory>
#include <mutex>
#include <filesystem>
#include <iostream>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;
using json   = nlohmann::json;

// ──────────────────────────────────────────────────────────────────────────────
// Thread-safe WebView dispatcher
// ──────────────────────────────────────────────────────────────────────────────
struct DispatchPayload {
    webview::webview* wv;
    std::string       jsCode;
};

static void dispatchCallback(webview::webview* w, void* arg) {
    auto* payload = static_cast<DispatchPayload*>(arg);
    payload->wv->eval(payload->jsCode);
    delete payload;
}

// Safely evaluate JS from any thread
static void evalFromThread(webview::webview* wv, const std::string& js) {
    auto* payload   = new DispatchPayload{wv, js};
    wv->dispatch([payload]() {
        payload->wv->eval(payload->jsCode);
        delete payload;
    });
}

// Push a JSON event object to the Vue layer
static void pushNativeMessage(webview::webview* wv, const json& msg) {
    std::string escaped = msg.dump();
    // Escape backticks for JS template literal safety
    std::string safe;
    safe.reserve(escaped.size());
    for (char c : escaped) {
        if (c == '`')  safe += "\\`";
        else if (c == '\\' && &c != escaped.data() + escaped.size() - 1) safe += c;
        else safe += c;
    }
    std::string js = "if(window.onNativeMessage){window.onNativeMessage(" + escaped + ");}";
    evalFromThread(wv, js);
}

// ──────────────────────────────────────────────────────────────────────────────
// App state
// ──────────────────────────────────────────────────────────────────────────────
struct AppState {
    webview::webview* wv{nullptr};
    Downloader        downloader;
    Launcher          launcher;
};

// ──────────────────────────────────────────────────────────────────────────────
// File-system scanner: checks if game exe exists in target directory
// ──────────────────────────────────────────────────────────────────────────────
static bool isGameInstalled(const std::string& directory, const std::string& exeName) {
    fs::path exePath = fs::path(directory) / exeName;
    return fs::exists(exePath);
}

// ──────────────────────────────────────────────────────────────────────────────
// Incoming message handler (JS → C++)
// ──────────────────────────────────────────────────────────────────────────────
static void handleMessage(AppState* app, const std::string& rawJson) {
    json msg;
    try {
        msg = json::parse(rawJson);
    } catch (...) {
        std::cerr << "[Bridge] JSON parse error: " << rawJson << "\n";
        return;
    }

    std::string action = msg.value("action", "");
    std::string gameId = msg.value("gameId", "");
    json        payload = msg.value("payload", json::object());

    if (action == "START_DOWNLOAD") {
        std::string url  = payload.value("url", "");
        std::string dest = payload.value("targetDirectory", "");
        if (url.empty() || dest.empty() || gameId.empty()) return;

        bool started = app->downloader.start(gameId, url, dest);
        if (!started) {
            json resp;
            resp["event"]          = "DOWNLOAD_ERROR";
            resp["gameId"]         = gameId;
            resp["data"]["status"] = "error";
            resp["data"]["message"] = "Download already in progress";
            pushNativeMessage(app->wv, resp);
        }

    } else if (action == "PAUSE_DOWNLOAD") {
        app->downloader.pause();

    } else if (action == "RESUME_DOWNLOAD") {
        app->downloader.resume();

    } else if (action == "CANCEL_DOWNLOAD") {
        app->downloader.cancel();

    } else if (action == "LAUNCH_GAME") {
        std::string execPath = payload.value("executablePath", "");
        std::string workDir  = payload.value("workingDirectory", "");
        if (execPath.empty() || gameId.empty()) return;

        bool launched = app->launcher.launch(gameId, execPath, workDir);
        if (!launched) {
            json resp;
            resp["event"]           = "GAME_EXECUTION_STATUS";
            resp["gameId"]          = gameId;
            resp["data"]["status"]  = "error";
            resp["data"]["message"] = "Game already running";
            pushNativeMessage(app->wv, resp);
        }

    } else if (action == "CHECK_INSTALLED") {
        std::string dir     = payload.value("directory", "");
        std::string exeName = payload.value("executableName", "");
        bool installed = isGameInstalled(dir, exeName);

        json resp;
        resp["event"]               = "INSTALL_STATUS";
        resp["gameId"]              = gameId;
        resp["data"]["installed"]   = installed;
        resp["data"]["status"]      = installed ? "installed" : "idle";
        pushNativeMessage(app->wv, resp);

    } else if (action == "SCAN_LIBRARY") {
        // Scan a directory for installed games
        std::string baseDir = payload.value("baseDirectory", "");
        json games = json::array();
        if (!baseDir.empty() && fs::exists(baseDir)) {
            for (const auto& entry : fs::directory_iterator(baseDir)) {
                if (entry.is_directory()) {
                    games.push_back(entry.path().filename().string());
                }
            }
        }
        json resp;
        resp["event"]          = "LIBRARY_SCAN_RESULT";
        resp["data"]["games"]  = games;
        resp["data"]["baseDir"] = baseDir;
        pushNativeMessage(app->wv, resp);

    } else {
        std::cerr << "[Bridge] Unknown action: " << action << "\n";
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Resolve UI directory path
// ──────────────────────────────────────────────────────────────────────────────
static std::string resolveUIPath() {
    // 1. Alongside executable: ./ui/index.html
    fs::path exeDir;
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    exeDir = fs::path(buf).parent_path();
#else
    exeDir = fs::canonical("/proc/self/exe").parent_path();
#endif

    fs::path candidate = exeDir / "ui" / "index.html";
    if (fs::exists(candidate))
        return candidate.string();

    // 2. Development fallback: ../app-ui/.output/public/index.html
    candidate = exeDir / ".." / "app-ui" / ".output" / "public" / "index.html";
    if (fs::exists(candidate))
        return fs::canonical(candidate).string();

    return "";
}

// ──────────────────────────────────────────────────────────────────────────────
// main
// ──────────────────────────────────────────────────────────────────────────────
int main() {
    auto app = std::make_unique<AppState>();

    // ── Create WebView ──────────────────────────────────────────────────────
    webview::webview wv(false, nullptr);
    app->wv = &wv;

    wv.set_title("Steam Clone - Game Launcher");
    wv.set_size(1280, 800, WEBVIEW_HINT_MIN);

    // ── Register JS → C++ bridge ────────────────────────────────────────────
    // window.sendToNative(jsonString) from Vue calls this binding
    wv.bind("sendToNative", [&app](const std::string& /*seq*/,
                                    const std::string& req,
                                    void* /*arg*/) -> std::string {
        // req is a JSON array: ["<actual payload>"]
        json args;
        try { args = json::parse(req); } catch (...) { return ""; }
        if (args.is_array() && !args.empty())
            handleMessage(app.get(), args[0].get<std::string>());
        return "";
    });

    // Inject initialization JS before page loads
    wv.init(R"js(
        // Polyfill: expose sendToNative as a simple function
        window.__nativeReady = false;
        window.__pendingMessages = [];

        window.postToNative = function(obj) {
            const str = JSON.stringify(obj);
            if (typeof sendToNative === 'function') {
                sendToNative(str);
            } else {
                window.__pendingMessages.push(str);
            }
        };

        // Flush pending messages once native bridge is ready
        window.addEventListener('DOMContentLoaded', function() {
            window.__nativeReady = true;
            if (window.__pendingMessages && window.__pendingMessages.length > 0) {
                window.__pendingMessages.forEach(function(m) {
                    try { sendToNative(m); } catch(e) {}
                });
                window.__pendingMessages = [];
            }
        });
    )js");

    // ── Register download progress callback ─────────────────────────────────
    app->downloader.setProgressCallback([&app](const DownloadProgress& p) {
        std::string statusStr;
        switch (p.state) {
            case DownloadState::Downloading: statusStr = "downloading"; break;
            case DownloadState::Paused:      statusStr = "paused";      break;
            case DownloadState::Completed:   statusStr = "installed";   break;
            case DownloadState::Error:       statusStr = "error";       break;
            case DownloadState::Cancelled:   statusStr = "idle";        break;
            default:                         statusStr = "idle";        break;
        }

        json msg;
        msg["event"]                          = "DOWNLOAD_PROGRESS_UPDATE";
        msg["gameId"]                         = p.gameId;
        msg["data"]["status"]                 = statusStr;
        msg["data"]["progressPercentage"]     = p.progressPercentage;
        msg["data"]["downloadSpeed"]          =
            std::to_string(static_cast<int>(p.downloadSpeedMBps * 10) / 10.0) + " MB/s";
        msg["data"]["estimatedTimeArrival"]   = p.eta;
        msg["data"]["bytesDownloaded"]        = p.bytesDownloaded;
        msg["data"]["totalBytes"]             = p.totalBytes;
        if (!p.errorMessage.empty())
            msg["data"]["errorMessage"] = p.errorMessage;

        pushNativeMessage(app->wv, msg);
    });

    // ── Register launcher status callback ───────────────────────────────────
    app->launcher.setStatusCallback([&app](const GameExecutionStatus& s) {
        std::string statusStr;
        switch (s.status) {
            case GameStatus::Launching: statusStr = "launching"; break;
            case GameStatus::Playing:   statusStr = "playing";   break;
            case GameStatus::Exited:    statusStr = "installed"; break;
            case GameStatus::Error:     statusStr = "error";     break;
            default:                    statusStr = "idle";      break;
        }

        json msg;
        msg["event"]              = "GAME_EXECUTION_STATUS";
        msg["gameId"]             = s.gameId;
        msg["data"]["status"]     = statusStr;
        msg["data"]["exitCode"]   = s.exitCode;
        if (!s.errorMessage.empty())
            msg["data"]["errorMessage"] = s.errorMessage;

        pushNativeMessage(app->wv, msg);
    });

    // ── Navigate to UI ──────────────────────────────────────────────────────
    std::string uiPath = resolveUIPath();
    if (!uiPath.empty()) {
#ifdef _WIN32
        // WebView2 requires file:/// URLs
        std::string fileUrl = "file:///" + uiPath;
        std::replace(fileUrl.begin(), fileUrl.end(), '\\', '/');
        wv.navigate(fileUrl);
#else
        wv.navigate("file://" + uiPath);
#endif
    } else {
        // Fallback: serve a minimal placeholder page
        wv.set_html(R"html(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Steam Clone</title>
<style>
  body { background: #1b2838; color: #c7d5e0; font-family: sans-serif;
         display: flex; align-items: center; justify-content: center;
         height: 100vh; margin: 0; flex-direction: column; gap: 16px; }
  h1   { font-size: 2rem; color: #66c0f4; }
  p    { opacity: 0.7; }
</style>
</head>
<body>
  <h1>Steam Clone</h1>
  <p>UI not found. Build the app-ui with <code>npm run generate</code>.</p>
  <p>Expected path: <code>./ui/index.html</code></p>
</body>
</html>
        )html");
    }

    // ── Run the event loop ──────────────────────────────────────────────────
    wv.run();
    return 0;
}
