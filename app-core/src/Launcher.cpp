#include "Launcher.h"

#include <iostream>
#include <stdexcept>

#ifndef _WIN32
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <cstring>
#endif

Launcher::Launcher() = default;

Launcher::~Launcher() {
    // Signal the monitor thread to stop
    {
        std::lock_guard<std::mutex> lock(launchMtx);
#ifdef _WIN32
        if (processHandle != INVALID_HANDLE_VALUE) {
            TerminateProcess(processHandle, 0);
            CloseHandle(processHandle);
            processHandle = INVALID_HANDLE_VALUE;
            CloseHandle(threadHandle);
            threadHandle = INVALID_HANDLE_VALUE;
        }
#else
        if (childPid > 0) {
            kill(childPid, SIGTERM);
            childPid = -1;
        }
#endif
    }
    if (monitorWorker.joinable())
        monitorWorker.join();
}

void Launcher::setStatusCallback(GameStatusCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMtx);
    statusCallback = std::move(cb);
}

void Launcher::notifyStatus(const GameExecutionStatus& status) {
    std::lock_guard<std::mutex> lock(callbackMtx);
    if (statusCallback)
        statusCallback(status);
}

bool Launcher::isRunning() const {
    return running.load();
}

std::string Launcher::getRunningGameId() const {
    std::lock_guard<std::mutex> lock(launchMtx);
    return runningGameId;
}

bool Launcher::launch(const std::string& gameId,
                      const std::string& executablePath,
                      const std::string& workingDirectory) {
    if (running.load()) return false;

    {
        std::lock_guard<std::mutex> lock(launchMtx);
        runningGameId = gameId;
    }

    // Notify launching
    GameExecutionStatus launchStatus{};
    launchStatus.gameId = gameId;
    launchStatus.status = GameStatus::Launching;
    notifyStatus(launchStatus);

#ifdef _WIN32
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    // Convert paths to wide strings
    auto toWide = [](const std::string& s) -> std::wstring {
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring ws(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
        return ws;
    };

    std::wstring wExec = toWide(executablePath);
    std::wstring wDir  = toWide(workingDirectory.empty() ? "." : workingDirectory);

    if (!CreateProcessW(
            wExec.c_str(),
            nullptr,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            wDir.c_str(),
            &si,
            &pi)) {
        GameExecutionStatus errStatus{};
        errStatus.gameId        = gameId;
        errStatus.status        = GameStatus::Error;
        errStatus.errorMessage  = "CreateProcess failed: " + std::to_string(GetLastError());
        notifyStatus(errStatus);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(launchMtx);
        processHandle = pi.hProcess;
        threadHandle  = pi.hThread;
    }

#else
    // Fork-exec on Linux/macOS
    pid_t pid = fork();
    if (pid < 0) {
        GameExecutionStatus errStatus{};
        errStatus.gameId       = gameId;
        errStatus.status       = GameStatus::Error;
        errStatus.errorMessage = std::string("fork() failed: ") + strerror(errno);
        notifyStatus(errStatus);
        return false;
    }

    if (pid == 0) {
        // Child process
        if (!workingDirectory.empty()) {
            if (chdir(workingDirectory.c_str()) != 0) {
                _exit(127);
            }
        }
        execl(executablePath.c_str(), executablePath.c_str(), nullptr);
        // If execl returns, there was an error
        _exit(126);
    }

    {
        std::lock_guard<std::mutex> lock(launchMtx);
        childPid = pid;
    }
#endif

    running.store(true);

    // Notify playing
    GameExecutionStatus playStatus{};
    playStatus.gameId = gameId;
    playStatus.status = GameStatus::Playing;
    notifyStatus(playStatus);

    // Start monitor thread
    if (monitorWorker.joinable())
        monitorWorker.join();
    monitorWorker = std::thread(&Launcher::monitorThread, this, gameId);

    return true;
}

void Launcher::monitorThread(std::string gameId) {
    int exitCode = 0;

#ifdef _WIN32
    HANDLE hProc;
    {
        std::lock_guard<std::mutex> lock(launchMtx);
        hProc = processHandle;
    }
    if (hProc != INVALID_HANDLE_VALUE) {
        WaitForSingleObject(hProc, INFINITE);
        DWORD code = 0;
        GetExitCodeProcess(hProc, &code);
        exitCode = static_cast<int>(code);

        std::lock_guard<std::mutex> lock(launchMtx);
        CloseHandle(processHandle);
        processHandle = INVALID_HANDLE_VALUE;
        CloseHandle(threadHandle);
        threadHandle = INVALID_HANDLE_VALUE;
    }
#else
    pid_t pid;
    {
        std::lock_guard<std::mutex> lock(launchMtx);
        pid = childPid;
    }
    if (pid > 0) {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            exitCode = WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            exitCode = -WTERMSIG(status);

        std::lock_guard<std::mutex> lock(launchMtx);
        childPid = -1;
    }
#endif

    running.store(false);

    GameExecutionStatus exitStatus{};
    exitStatus.gameId   = gameId;
    exitStatus.status   = GameStatus::Exited;
    exitStatus.exitCode = exitCode;
    notifyStatus(exitStatus);
}
