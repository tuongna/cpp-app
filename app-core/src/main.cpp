#include <webview/webview.h>
#include "Downloader.h"
#include "Launcher.h"
#include "httplib.h"

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
// Simple JSON builder for message passing
// ──────────────────────────────────────────────────────────────────────────────
class SimpleJSON {
public:
    SimpleJSON() : isObject(true) {}
    
    SimpleJSON& set(const std::string& key, const std::string& value) {
        stringValues[key] = escapeString(value);
        return *this;
    }
    
    SimpleJSON& set(const std::string& key, int value) {
        intValues[key] = value;
        return *this;
    }
    
    SimpleJSON& set(const std::string& key, double value) {
        doubleValues[key] = value;
        return *this;
    }
    
    SimpleJSON& set(const std::string& key, long long value) {
        longValues[key] = value;
        return *this;
    }
    
    SimpleJSON& set(const std::string& key, bool value) {
        boolValues[key] = value;
        return *this;
    }
    
    SimpleJSON& setArray(const std::string& key, const std::vector<std::string>& arr) {
        arrayValues[key] = arr;
        return *this;
    }
    
    SimpleJSON& setObject(const std::string& key, const SimpleJSON& obj) {
        objectValues[key] = obj;
        return *this;
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        
        for (const auto& [key, value] : stringValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        for (const auto& [key, value] : intValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << value;
            first = false;
        }
        for (const auto& [key, value] : doubleValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << value;
            first = false;
        }
        for (const auto& [key, value] : longValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << value;
            first = false;
        }
        for (const auto& [key, value] : boolValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << (value ? "true" : "false");
            first = false;
        }
        for (const auto& [key, arr] : arrayValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":[";
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) oss << ",";
                oss << "\"" << escapeString(arr[i]) << "\"";
            }
            oss << "]";
            first = false;
        }
        for (const auto& [key, obj] : objectValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << obj.toString();
            first = false;
        }
        
        oss << "}";
        return oss.str();
    }
    
private:
    static std::string escapeString(const std::string& str) {
        std::string result;
        result.reserve(str.size());
        for (char c : str) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:   result += c; break;
            }
        }
        return result;
    }
    
    bool isObject;
    std::map<std::string, std::string> stringValues;
    std::map<std::string, int> intValues;
    std::map<std::string, double> doubleValues;
    std::map<std::string, long long> longValues;
    std::map<std::string, bool> boolValues;
    std::map<std::string, std::vector<std::string>> arrayValues;
    std::map<std::string, SimpleJSON> objectValues;
};

// Simple JSON parser for incoming messages
class SimpleJSONParser {
public:
    static std::map<std::string, std::string> parse(const std::string& json) {
        std::map<std::string, std::string> result;
        
        // Very basic parser - just handles simple key-value pairs
        size_t pos = 0;
        while (pos < json.size()) {
            // Find key
            size_t keyStart = json.find('"', pos);
            if (keyStart == std::string::npos) break;
            keyStart++;
            
            size_t keyEnd = json.find('"', keyStart);
            if (keyEnd == std::string::npos) break;
            
            std::string key = json.substr(keyStart, keyEnd - keyStart);
            
            // Find colon
            size_t colon = json.find(':', keyEnd);
            if (colon == std::string::npos) break;
            
            // Find value
            size_t valueStart = colon + 1;
            while (valueStart < json.size() && std::isspace(json[valueStart])) valueStart++;
            
            std::string value;
            if (json[valueStart] == '"') {
                // String value
                valueStart++;
                size_t valueEnd = json.find('"', valueStart);
                if (valueEnd != std::string::npos) {
                    value = json.substr(valueStart, valueEnd - valueStart);
                    pos = valueEnd + 1;
                }
            } else if (json[valueStart] == '{') {
                // Object value - find matching }
                int braceCount = 1;
                size_t i = valueStart + 1;
                while (i < json.size() && braceCount > 0) {
                    if (json[i] == '{') braceCount++;
                    else if (json[i] == '}') braceCount--;
                    i++;
                }
                value = json.substr(valueStart, i - valueStart);
                pos = i;
            } else {
                // Number or boolean
                size_t valueEnd = json.find_first_of(",}", valueStart);
                if (valueEnd != std::string::npos) {
                    value = json.substr(valueStart, valueEnd - valueStart);
                    // Trim
                    value.erase(0, value.find_first_not_of(" \t\n\r"));
                    value.erase(value.find_last_not_of(" \t\n\r") + 1);
                    pos = valueEnd;
                }
            }
            
            result[key] = value;
        }
        
        return result;
    }
    
    static std::string getValue(const std::map<std::string, std::string>& data, 
                               const std::string& key, 
                               const std::string& defaultValue = "") {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : defaultValue;
    }
};

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
static void pushNativeMessage(webview::webview* wv, const SimpleJSON& msg) {
    std::string escaped = msg.toString();
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
