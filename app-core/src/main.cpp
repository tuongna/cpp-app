// IMPORTANT: include httplib.h (and our own clean headers) BEFORE webview.h.
// On Linux, webview.h pulls in GTK -> X11, which #defines common words such as
// None, Status and Success. httplib.h uses those as identifiers, so including it
// after webview.h breaks compilation (the macros mangle httplib's namespace).
#include "httplib.h"
#include "Downloader.h"
#include "Launcher.h"
#include "Json.h"

#include <webview/webview.h>

#include <thread>

#include <string>
#include <memory>
#include <filesystem>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <map>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <limits.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif

// ──────────────────────────────────────────────────────────────────────────────
// Thread-safe WebView dispatcher
// ──────────────────────────────────────────────────────────────────────────────
// Marshal a JS eval onto the WebView's UI thread. Safe to call from any thread
// (e.g. the download/launcher worker threads).
static void evalFromThread(webview::webview* wv, const std::string& js) {
    wv->dispatch([wv, js]() {
        wv->eval(js);
    });
}

// Push a JSON event object to the Vue layer
static void pushNativeMessage(webview::webview* wv, const SimpleJSON& msg) {
    std::string js = "if(window.onNativeMessage){window.onNativeMessage(" + msg.toString() + ");}";
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
    auto msg = SimpleJSONParser::parse(rawJson);
    
    std::string action = SimpleJSONParser::getValue(msg, "action");
    std::string gameId = SimpleJSONParser::getValue(msg, "gameId");
    
    // Parse payload object if exists
    std::string payloadStr = SimpleJSONParser::getValue(msg, "payload");
    auto payload = SimpleJSONParser::parse(payloadStr);

    if (action == "START_DOWNLOAD") {
        std::string url  = SimpleJSONParser::getValue(payload, "url");
        std::string dest = SimpleJSONParser::getValue(payload, "targetDirectory");
        if (url.empty() || dest.empty() || gameId.empty()) return;

        bool started = app->downloader.start(gameId, url, dest);
        if (!started) {
            SimpleJSON data;
            data.set("status", "error")
                .set("message", "Download already in progress");
            
            SimpleJSON resp;
            resp.set("event", "DOWNLOAD_ERROR")
                .set("gameId", gameId)
                .setObject("data", data);
            
            pushNativeMessage(app->wv, resp);
        }

    } else if (action == "PAUSE_DOWNLOAD") {
        app->downloader.pause();

    } else if (action == "RESUME_DOWNLOAD") {
        app->downloader.resume();

    } else if (action == "CANCEL_DOWNLOAD") {
        app->downloader.cancel();

    } else if (action == "LAUNCH_GAME") {
        std::string execPath = SimpleJSONParser::getValue(payload, "executablePath");
        std::string workDir  = SimpleJSONParser::getValue(payload, "workingDirectory");
        if (execPath.empty() || gameId.empty()) return;

        bool launched = app->launcher.launch(gameId, execPath, workDir);
        if (!launched) {
            SimpleJSON data;
            data.set("status", "error")
                .set("message", "Game already running");
            
            SimpleJSON resp;
            resp.set("event", "GAME_EXECUTION_STATUS")
                .set("gameId", gameId)
                .setObject("data", data);
            
            pushNativeMessage(app->wv, resp);
        }

    } else if (action == "CHECK_INSTALLED") {
        std::string dir     = SimpleJSONParser::getValue(payload, "directory");
        std::string exeName = SimpleJSONParser::getValue(payload, "executableName");
        bool installed = isGameInstalled(dir, exeName);

        SimpleJSON data;
        data.set("installed", installed)
            .set("status", installed ? "installed" : "idle");
        
        SimpleJSON resp;
        resp.set("event", "INSTALL_STATUS")
            .set("gameId", gameId)
            .setObject("data", data);
        
        pushNativeMessage(app->wv, resp);

    } else if (action == "SCAN_LIBRARY") {
        // Scan a directory for installed games
        std::string baseDir = SimpleJSONParser::getValue(payload, "baseDirectory");
        std::vector<std::string> games;
        
        if (!baseDir.empty() && fs::exists(baseDir)) {
            for (const auto& entry : fs::directory_iterator(baseDir)) {
                if (entry.is_directory()) {
                    games.push_back(entry.path().filename().string());
                }
            }
        }
        
        SimpleJSON data;
        data.setArray("games", games)
            .set("baseDir", baseDir);
        
        SimpleJSON resp;
        resp.set("event", "LIBRARY_SCAN_RESULT")
            .setObject("data", data);
        
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
#elif defined(__APPLE__)
    // macOS: Use _NSGetExecutablePath
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        exeDir = fs::canonical(path).parent_path();
    } else {
        // Fallback: use current directory
        exeDir = fs::current_path();
    }
#else
    // Linux: Use /proc/self/exe
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
// ──────────────────────────────────────────────────────────────────────────────
int main() {
#ifdef __APPLE__
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
#endif
    auto app = std::make_unique<AppState>();

    // ── Create WebView ──────────────────────────────────────────────────────
    webview::webview wv(true, nullptr);
    app->wv = &wv;

    wv.set_title("Steam Clone - Game Launcher");
    wv.set_size(1280, 800, WEBVIEW_HINT_NONE);
    wv.set_size(1280, 800, WEBVIEW_HINT_MIN);

    // ── Register JS → C++ bridge ────────────────────────────────────────────
    // window.sendToNative(jsonString) from Vue calls this binding
    wv.bind("sendToNative", [&app](std::string req) -> std::string {
        // req is a JSON array: ["<actual payload>"]
        // Simple extraction of first parameter
        size_t start = req.find('"');
        if (start != std::string::npos) {
            start++; // Skip opening quote
            size_t end = req.rfind('"');
            if (end != std::string::npos && end > start) {
                std::string payload = req.substr(start, end - start);
                // Unescape the JSON string
                std::string unescaped;
                for (size_t i = 0; i < payload.size(); ++i) {
                    if (payload[i] == '\\' && i + 1 < payload.size()) {
                        char next = payload[i + 1];
                        if (next == '"' || next == '\\' || next == '/') {
                            unescaped += next;
                            i++;
                        } else if (next == 'n') {
                            unescaped += '\n';
                            i++;
                        } else if (next == 'r') {
                            unescaped += '\r';
                            i++;
                        } else if (next == 't') {
                            unescaped += '\t';
                            i++;
                        } else {
                            unescaped += payload[i];
                        }
                    } else {
                        unescaped += payload[i];
                    }
                }
                handleMessage(app.get(), unescaped);
            }
        }
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

        std::ostringstream speedStr;
        speedStr << std::fixed << std::setprecision(1) << p.downloadSpeedMBps << " MB/s";
        
        SimpleJSON data;
        data.set("status", statusStr)
            .set("progressPercentage", p.progressPercentage)
            .set("downloadSpeed", speedStr.str())
            .set("estimatedTimeArrival", p.eta)
            .set("bytesDownloaded", p.bytesDownloaded)
            .set("totalBytes", p.totalBytes);
        
        if (!p.errorMessage.empty())
            data.set("errorMessage", p.errorMessage);
        
        SimpleJSON msg;
        msg.set("event", "DOWNLOAD_PROGRESS_UPDATE")
           .set("gameId", p.gameId)
           .setObject("data", data);

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

        SimpleJSON data;
        data.set("status", statusStr)
            .set("exitCode", s.exitCode);
        
        if (!s.errorMessage.empty())
            data.set("errorMessage", s.errorMessage);
        
        SimpleJSON msg;
        msg.set("event", "GAME_EXECUTION_STATUS")
           .set("gameId", s.gameId)
           .setObject("data", data);

        pushNativeMessage(app->wv, msg);
    });

    // ── Navigate to UI ──────────────────────────────────────────────────────
    std::string uiIndex = resolveUIPath();
    if (!uiIndex.empty()) {
        std::string uiDir = fs::path(uiIndex).parent_path().string();
        
        // Host UI via a local HTTP server to avoid file:// protocol issues
        static httplib::Server svr;
        svr.set_mount_point("/", uiDir);
        
        int port = svr.bind_to_any_port("127.0.0.1");
        std::thread([]() {
            svr.listen_after_bind();
        }).detach();
        
        wv.navigate("http://127.0.0.1:" + std::to_string(port));
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
