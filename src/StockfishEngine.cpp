#include "StockfishEngine.hpp"

#include <sstream>

#ifdef _WIN32
#include <algorithm>
#endif

StockfishEngine::~StockfishEngine() {
    stop();
}

bool StockfishEngine::start(const std::string& executablePath) {
#ifndef _WIN32
    lastError = "Stockfish integration currently supports Windows builds only.";
    (void)executablePath;
    return false;
#else
    if (isRunning()) return true;

    enginePath = executablePath;
    pendingOutput.clear();
    lastError.clear();

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE childStdoutWrite = nullptr;
    HANDLE childStdinRead = nullptr;

    if (!CreatePipe(&childStdoutRead, &childStdoutWrite, &sa, 0)) {
        lastError = "Could not create Stockfish stdout pipe.";
        return false;
    }
    if (!SetHandleInformation(childStdoutRead, HANDLE_FLAG_INHERIT, 0)) {
        lastError = "Could not configure Stockfish stdout pipe.";
        CloseHandle(childStdoutRead);
        CloseHandle(childStdoutWrite);
        childStdoutRead = nullptr;
        return false;
    }

    if (!CreatePipe(&childStdinRead, &childStdinWrite, &sa, 0)) {
        lastError = "Could not create Stockfish stdin pipe.";
        CloseHandle(childStdoutRead);
        CloseHandle(childStdoutWrite);
        childStdoutRead = nullptr;
        return false;
    }
    if (!SetHandleInformation(childStdinWrite, HANDLE_FLAG_INHERIT, 0)) {
        lastError = "Could not configure Stockfish stdin pipe.";
        CloseHandle(childStdoutRead);
        CloseHandle(childStdoutWrite);
        CloseHandle(childStdinRead);
        CloseHandle(childStdinWrite);
        childStdoutRead = nullptr;
        childStdinWrite = nullptr;
        return false;
    }

    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(STARTUPINFOA);
    startupInfo.hStdError = childStdoutWrite;
    startupInfo.hStdOutput = childStdoutWrite;
    startupInfo.hStdInput = childStdinRead;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;

    std::string commandLine = "\"" + enginePath + "\"";
    BOOL ok = CreateProcessA(
        nullptr,
        commandLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo);

    CloseHandle(childStdoutWrite);
    CloseHandle(childStdinRead);

    if (!ok) {
        lastError = "Could not start Stockfish executable: " + enginePath;
        CloseHandle(childStdoutRead);
        CloseHandle(childStdinWrite);
        childStdoutRead = nullptr;
        childStdinWrite = nullptr;
        return false;
    }

    if (!sendCommand("uci") || !waitForToken("uciok", 5000)) {
        lastError = "Stockfish did not complete UCI startup.";
        stop();
        return false;
    }
    if (!sendCommand("isready") || !waitForToken("readyok", 5000)) {
        lastError = "Stockfish did not become ready.";
        stop();
        return false;
    }

    return true;
#endif
}

void StockfishEngine::stop() {
#ifdef _WIN32
    if (childStdinWrite) {
        sendCommand("quit");
    }
    if (processInfo.hProcess) {
        WaitForSingleObject(processInfo.hProcess, 500);
        DWORD exitCode = 0;
        if (GetExitCodeProcess(processInfo.hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
            TerminateProcess(processInfo.hProcess, 0);
        }
        CloseHandle(processInfo.hProcess);
        processInfo.hProcess = nullptr;
    }
    if (processInfo.hThread) {
        CloseHandle(processInfo.hThread);
        processInfo.hThread = nullptr;
    }
    if (childStdinWrite) {
        CloseHandle(childStdinWrite);
        childStdinWrite = nullptr;
    }
    if (childStdoutRead) {
        CloseHandle(childStdoutRead);
        childStdoutRead = nullptr;
    }
#endif
}

bool StockfishEngine::isRunning() const {
#ifdef _WIN32
    if (!processInfo.hProcess) return false;
    DWORD exitCode = 0;
    return GetExitCodeProcess(processInfo.hProcess, &exitCode) && exitCode == STILL_ACTIVE;
#else
    return false;
#endif
}

bool StockfishEngine::setSkillLevel(int level) {
    if (!isRunning() && !start(enginePath.empty() ? "stockfish/stockfish-windows-x86-64-avx2.exe" : enginePath)) {
        return false;
    }

    if (level < 0) level = 0;
    if (level > 20) level = 20;

    if (!sendCommand("setoption name Skill Level value " + std::to_string(level))) return false;
    if (!sendCommand("isready")) return false;
    return waitForToken("readyok", 5000);
}

std::string StockfishEngine::getBestMove(const std::vector<std::string>& uciMoves, int depth) {
    if (!isRunning() && !start(enginePath.empty() ? "stockfish/stockfish-windows-x86-64-avx2.exe" : enginePath)) {
        return "";
    }

    std::ostringstream position;
    position << "position startpos";
    if (!uciMoves.empty()) {
        position << " moves";
        for (const auto& move : uciMoves) {
            position << " " << move;
        }
    }

    if (!sendCommand(position.str())) return "";
    if (!sendCommand("go depth " + std::to_string(depth))) return "";

    std::string line;
    while (readLine(line, 10000)) {
        if (line.rfind("bestmove ", 0) == 0) {
            std::istringstream ss(line);
            std::string label;
            std::string move;
            ss >> label >> move;
            if (move == "(none)" || move == "0000") return "";
            return move;
        }
    }

    lastError = "Stockfish did not return a bestmove.";
    return "";
}

bool StockfishEngine::sendCommand(const std::string& command) {
#ifndef _WIN32
    (void)command;
    return false;
#else
    if (!childStdinWrite) {
        lastError = "Stockfish stdin pipe is not open.";
        return false;
    }

    std::string text = command + "\n";
    DWORD written = 0;
    if (!WriteFile(childStdinWrite, text.c_str(), (DWORD)text.size(), &written, nullptr)) {
        lastError = "Could not send command to Stockfish.";
        return false;
    }
    return written == text.size();
#endif
}

bool StockfishEngine::waitForToken(const std::string& token, unsigned long timeoutMs) {
#ifndef _WIN32
    (void)token;
    (void)timeoutMs;
    return false;
#else
    std::string line;
    unsigned long startTick = GetTickCount();
    while (GetTickCount() - startTick < timeoutMs) {
        if (readLine(line, 100)) {
            if (line.find(token) != std::string::npos) return true;
        }
    }
    return false;
#endif
}

bool StockfishEngine::readLine(std::string& line, unsigned long timeoutMs) {
#ifndef _WIN32
    (void)line;
    (void)timeoutMs;
    return false;
#else
    line.clear();
    unsigned long startTick = GetTickCount();

    while (GetTickCount() - startTick < timeoutMs) {
        size_t newline = pendingOutput.find('\n');
        if (newline != std::string::npos) {
            line = pendingOutput.substr(0, newline);
            pendingOutput.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return true;
        }

        DWORD available = 0;
        if (!PeekNamedPipe(childStdoutRead, nullptr, 0, nullptr, &available, nullptr)) {
            lastError = "Could not read from Stockfish stdout.";
            return false;
        }

        if (available > 0) {
            char buffer[512];
            DWORD bytesRead = 0;
            DWORD toRead = std::min<DWORD>(available, sizeof(buffer));
            if (!ReadFile(childStdoutRead, buffer, toRead, &bytesRead, nullptr) || bytesRead == 0) {
                lastError = "Stockfish stdout pipe closed unexpectedly.";
                return false;
            }
            pendingOutput.append(buffer, buffer + bytesRead);
        } else {
            Sleep(5);
        }
    }

    return false;
#endif
}
