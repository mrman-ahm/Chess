#include "Game.hpp"

#ifdef _WIN32
#include <windows.h>

#include <filesystem>
#include <system_error>

namespace {

void useExecutableDirectory() {
    std::wstring path(32768, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) return;

    path.resize(length);
    std::error_code error;
    std::filesystem::current_path(std::filesystem::path(path).parent_path(), error);
}

} // namespace
#endif

int main() {
#ifdef _WIN32
    useExecutableDirectory();
#endif
    Game game;
    game.run();
    return 0;
}
