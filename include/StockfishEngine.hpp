#pragma once

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

class StockfishEngine {
public:
    StockfishEngine() = default;
    ~StockfishEngine();

    bool start(const std::string& executablePath);
    void stop();
    bool isRunning() const;
    bool setSkillLevel(int level);
    std::string getBestMove(const std::vector<std::string>& uciMoves, int depth);
    const std::string& getLastError() const { return lastError; }

private:
    bool sendCommand(const std::string& command);
    bool waitForToken(const std::string& token, unsigned long timeoutMs);
    bool readLine(std::string& line, unsigned long timeoutMs);

    std::string enginePath;
    std::string pendingOutput;
    std::string lastError;

#ifdef _WIN32
    PROCESS_INFORMATION processInfo{};
    HANDLE childStdinWrite = nullptr;
    HANDLE childStdoutRead = nullptr;
#endif
};
