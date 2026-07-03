# Sixty-Four

<p align="center">
  <a href="https://github.com/mrman-ahm/Chess/releases/latest/download/Sixty-Four-Windows.zip">
    <img src="https://img.shields.io/badge/DOWNLOAD_FOR_WINDOWS-SIXTY--FOUR-2f81f7?style=for-the-badge&logo=windows" alt="Download Sixty-Four for Windows">
  </a>
</p>

> [!IMPORTANT]
> **Playable Windows download:** [Download Sixty-Four-Windows.zip](https://github.com/mrman-ahm/Chess/releases/latest/download/Sixty-Four-Windows.zip), extract it, and double-click `sixty-four.exe`. No installation or compiler is required.

Sixty-Four is a customizable Windows chess game built in C++17 with SFML. It was created by Muhammad Ahmad as a student game-development project and exhibition piece.

## Features

- Local two-player chess
- Stockfish-powered computer opponent with selectable difficulty
- Standard chess, Chess960, Crazyhouse, Three-check, and King of the Hill
- Drag-and-drop and click-to-move controls
- Save/load support using PGN files
- Move history, undo/redo review, clocks, increments, and delays
- Fifteen selectable piece sets with per-piece customization
- Board palettes, game backgrounds, menu presets, avatars, and cursors
- Menu/game sound controls and an optional FAHH sound mode
- Fullscreen and windowed display modes

Multiplayer is shown as unavailable and is not implemented in this version.

## Requirements

- Windows 10 or Windows 11, 64-bit
- MinGW-w64 with `g++` and `mingw32-make`
- Git LFS for the bundled Stockfish executable

The repository includes the SFML 2.6.1 headers, import libraries, and runtime DLLs used by the project.

## Build

```powershell
git lfs install
git lfs pull
mingw32-make
```

Run from the repository root so runtime asset paths resolve correctly:

```powershell
mingw32-make run
```

Create a standalone playable release from this source repository:

```powershell
mingw32-make package
```

The source version is this repository itself. The playable package is written
to `dist/Sixty-Four Release` and starts by double-clicking `sixty-four.exe`;
it resolves all assets relative to its own folder.

`mingw32-make package` also creates `dist/Sixty-Four-Windows.zip`. Upload that
file to a GitHub Release with the exact same filename so the download button at
the top of this README always points to the latest published build.

## Controls

- Mouse: navigate menus, drag pieces, or click source and destination squares
- Arrow keys or `W`/`S`: navigate the main menu
- Enter or Space: activate the selected menu option
- Escape: back, close a modal, or open the in-game session menu

## Project Structure

- `src/` and `include/`: game source code
- `Sprites/`: piece sets, backgrounds, fonts, avatars, cursors, and UI images
- `Audio/Moves/`: runtime sound effects
- `external/SFML/`: bundled SFML development/runtime files
- `stockfish/`: bundled Stockfish executable, source, and license
- `data/`: runtime-generated configuration and saved matches

## Credits

Design, development, programming, testing, and project direction: **Muhammad Ahmad**.

Third-party software and known asset credits are documented in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## License

The original Sixty-Four source code is source-available for educational review. See [LICENSE](LICENSE). Third-party components and assets remain under their respective licenses.
