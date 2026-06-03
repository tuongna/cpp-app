#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

enum class GameStatus {
    Idle,
    Launching,
    Playing,
    Exited,
    Error
};

struct GameExecutionStatus {
    std::string gameId;
    GameStatus  status;
    int         exitCode;
    std::string errorMessage;
};

using GameStatusCallback = std::function<void(const GameExecutionStatus&)>;

class Launcher {
public:
    Launcher();
    ~Launcher();

    void setStatusCallback(GameStatusCallback cb);

    // Launch a game executable; returns false if a game is already running
    bool launch(const std::string& gameId,
                const std::string& executablePath,
                const std::string& workingDirectory);

    bool        isRunning() const;
    std::string getRunningGameId() const;

private:
    void monitorThread(std::string gameId);
    void notifyStatus(const GameExecutionStatus& status);

    std::thread          monitorWorker;
    std::atomic<bool>    running{false};
    mutable std::mutex   launchMtx;
    std::string          runningGameId;
    GameStatusCallback   statusCallback;
    std::mutex           callbackMtx;

#ifdef _WIN32
    HANDLE processHandle{INVALID_HANDLE_VALUE};
    HANDLE threadHandle{INVALID_HANDLE_VALUE};
#else
    pid_t childPid{-1};
#endif
};
