#include "Downloader.h"

// NOTE: Do NOT define CPPHTTPLIB_OPENSSL_SUPPORT here.
// httplib.h uses #ifdef (not #if), so even defining it to 0 enables OpenSSL.
#include "httplib.h"


#include <filesystem>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace fs = std::filesystem;

Downloader::Downloader() = default;

Downloader::~Downloader() {
    cancel();
    if (workerThread.joinable())
        workerThread.join();
}

void Downloader::setProgressCallback(DownloadProgressCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMtx);
    progressCallback = std::move(cb);
}

bool Downloader::start(const std::string& gameId,
                       const std::string& url,
                       const std::string& destPath) {
    if (state.load() == DownloadState::Downloading ||
        state.load() == DownloadState::Paused) {
        return false;
    }

    if (workerThread.joinable())
        workerThread.join();

    cancelFlag.store(false);
    pauseFlag.store(false);
    state.store(DownloadState::Downloading);

    {
        std::lock_guard<std::mutex> lock(stateMtx);
        currentGameId = gameId;
    }

    workerThread = std::thread(&Downloader::downloadThread, this, gameId, url, destPath);
    return true;
}

void Downloader::pause() {
    if (state.load() == DownloadState::Downloading) {
        pauseFlag.store(true);
        state.store(DownloadState::Paused);
    }
}

void Downloader::resume() {
    if (state.load() == DownloadState::Paused) {
        pauseFlag.store(false);
        state.store(DownloadState::Downloading);
        pauseCV.notify_all();
    }
}

void Downloader::cancel() {
    cancelFlag.store(true);
    pauseFlag.store(false);
    pauseCV.notify_all();
}

DownloadState Downloader::getState() const {
    return state.load();
}

std::string Downloader::getCurrentGameId() const {
    std::lock_guard<std::mutex> lock(stateMtx);
    return currentGameId;
}

void Downloader::notifyProgress(const DownloadProgress& progress) {
    std::lock_guard<std::mutex> lock(callbackMtx);
    if (progressCallback)
        progressCallback(progress);
}

std::string Downloader::formatETA(long long seconds) const {
    if (seconds < 0) return "--:--:--";
    long long h = seconds / 3600;
    long long m = (seconds % 3600) / 60;
    long long s = seconds % 60;
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << h << ":"
        << std::setw(2) << m << ":"
        << std::setw(2) << s;
    return oss.str();
}

void Downloader::downloadThread(std::string gameId, std::string url, std::string destPath) {
    // Parse URL into host + path
    std::string host, path;
    int port = 80;
    bool useHttps = false;

    auto stripScheme = [&](const std::string& u) -> std::string {
        if (u.substr(0, 8) == "https://") { useHttps = true; return u.substr(8); }
        if (u.substr(0, 7) == "http://")  { return u.substr(7); }
        return u;
    };

    std::string rest = stripScheme(url);
    auto slashPos = rest.find('/');
    if (slashPos != std::string::npos) {
        host = rest.substr(0, slashPos);
        path = rest.substr(slashPos);
    } else {
        host = rest;
        path = "/";
    }

    // Handle port in host
    auto colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        port = std::stoi(host.substr(colonPos + 1));
        host = host.substr(0, colonPos);
    } else {
        port = useHttps ? 443 : 80;
    }

    // Ensure destination directory exists
    fs::path dest(destPath);
    fs::create_directories(dest.parent_path());

    // Check for resume: get existing file size
    long long resumeFrom = 0;
    if (fs::exists(destPath)) {
        resumeFrom = static_cast<long long>(fs::file_size(destPath));
    }

    httplib::Client cli(host, port);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(30, 0);

    // First get total file size via HEAD
    long long totalSize = -1;
    auto headRes = cli.Head(path);
    if (headRes && headRes->has_header("Content-Length")) {
        totalSize = std::stoll(headRes->get_header_value("Content-Length"));
    }

    // Open file for writing (append if resuming)
    std::ofstream outFile(destPath, resumeFrom > 0
                          ? std::ios::binary | std::ios::app
                          : std::ios::binary);
    if (!outFile.is_open()) {
        state.store(DownloadState::Error);
        DownloadProgress progress{};
        progress.gameId = gameId;
        progress.state = DownloadState::Error;
        progress.errorMessage = "Failed to open destination file: " + destPath;
        notifyProgress(progress);
        return;
    }

    long long downloaded = resumeFrom;
    auto startTime = std::chrono::steady_clock::now();
    auto lastSpeedCalc = startTime;
    long long lastBytes = downloaded;
    double currentSpeed = 0.0;

    // Set Range header for resume
    httplib::Headers headers;
    if (resumeFrom > 0)
        headers.emplace("Range", "bytes=" + std::to_string(resumeFrom) + "-");

    auto res = cli.Get(path, headers,
        [&](const char* data, size_t dataLen) -> bool {
            // Check for cancellation
            if (cancelFlag.load()) return false;

            // Handle pause
            if (pauseFlag.load()) {
                std::unique_lock<std::mutex> lock(stateMtx);
                pauseCV.wait(lock, [this] {
                    return !pauseFlag.load() || cancelFlag.load();
                });
                if (cancelFlag.load()) return false;
            }

            outFile.write(data, static_cast<std::streamsize>(dataLen));
            downloaded += static_cast<long long>(dataLen);

            // Update total size if we didn't get it from HEAD
            if (totalSize < 0 && resumeFrom == 0)
                totalSize = downloaded; // best effort

            // Calculate speed every 500ms
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - lastSpeedCalc).count();
            if (elapsed >= 0.5) {
                currentSpeed = static_cast<double>(downloaded - lastBytes)
                               / elapsed / (1024.0 * 1024.0);
                lastBytes = downloaded;
                lastSpeedCalc = now;
            }

            double pct = totalSize > 0
                ? std::min(100.0, static_cast<double>(downloaded) / totalSize * 100.0)
                : -1.0;

            long long eta = -1;
            if (totalSize > 0 && currentSpeed > 0) {
                long long remaining = totalSize - downloaded;
                eta = static_cast<long long>(
                    (remaining / (1024.0 * 1024.0)) / currentSpeed);
            }

            DownloadProgress progress{};
            progress.gameId            = gameId;
            progress.state             = state.load();
            progress.progressPercentage = pct;
            progress.downloadSpeedMBps  = currentSpeed;
            progress.eta               = formatETA(eta);
            progress.bytesDownloaded   = downloaded;
            progress.totalBytes        = totalSize;
            notifyProgress(progress);

            return true;
        }
    );

    outFile.close();

    if (cancelFlag.load()) {
        state.store(DownloadState::Cancelled);
        DownloadProgress p{};
        p.gameId = gameId;
        p.state  = DownloadState::Cancelled;
        notifyProgress(p);
        return;
    }

    if (!res || (res->status != 200 && res->status != 206)) {
        state.store(DownloadState::Error);
        DownloadProgress p{};
        p.gameId       = gameId;
        p.state        = DownloadState::Error;
        p.errorMessage = res ? ("HTTP " + std::to_string(res->status)) : "Connection failed";
        notifyProgress(p);
        return;
    }

    state.store(DownloadState::Completed);
    DownloadProgress p{};
    p.gameId             = gameId;
    p.state              = DownloadState::Completed;
    p.progressPercentage = 100.0;
    p.bytesDownloaded    = downloaded;
    p.totalBytes         = totalSize;
    p.downloadSpeedMBps  = 0.0;
    p.eta                = "00:00:00";
    notifyProgress(p);
}
