#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <fstream>
#include <chrono>

enum class DownloadState {
    Idle,
    Downloading,
    Paused,
    Completed,
    Error,
    Cancelled
};

struct DownloadProgress {
    std::string gameId;
    DownloadState state;
    double progressPercentage;   // 0.0 - 100.0
    double downloadSpeedMBps;    // MB/s
    std::string eta;             // "HH:MM:SS"
    long long bytesDownloaded;
    long long totalBytes;
    std::string errorMessage;
};

using DownloadProgressCallback = std::function<void(const DownloadProgress&)>;

class Downloader {
public:
    Downloader();
    ~Downloader();

    void setProgressCallback(DownloadProgressCallback cb);

    // Start a download; returns false if already downloading
    bool start(const std::string& gameId,
               const std::string& url,
               const std::string& destPath);

    void pause();
    void resume();
    void cancel();

    DownloadState getState() const;
    std::string getCurrentGameId() const;

private:
    void downloadThread(std::string gameId, std::string url, std::string destPath);
    std::string formatETA(long long remainingSeconds) const;
    void notifyProgress(const DownloadProgress& progress);

    std::thread                  workerThread;
    mutable std::mutex           stateMtx;
    std::condition_variable      pauseCV;
    std::atomic<bool>            pauseFlag{false};
    std::atomic<bool>            cancelFlag{false};
    std::atomic<DownloadState>   state{DownloadState::Idle};
    std::string                  currentGameId;
    DownloadProgressCallback     progressCallback;
    std::mutex                   callbackMtx;
};
