#include "Game.hpp"
#include "UIPrimitives.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cmath>
#include <cctype>
#include <random>

static float parseBaseTime(const std::string& str) {
    if (str == "1 Min") return 60.f;
    if (str == "3 Min") return 180.f;
    if (str == "5 Min") return 300.f;
    if (str == "10 Min") return 600.f;
    if (str == "15 Min") return 900.f;
    if (str == "30 Min") return 1800.f;
    if (str == "60 Min") return 3600.f;
    return 600.f;
}

static float parseSeconds(const std::string& str) {
    try {
        size_t space = str.find(' ');
        if (space != std::string::npos) {
            return std::stof(str.substr(0, space));
        }
    } catch (...) {}
    return 0.f;
}

static int parseAISkillLevel(const std::string& str) {
    try {
        int level = std::stoi(str);
        return std::clamp(level, 0, 20);
    } catch (...) {
        return 5;
    }
}

static int getDepthForAISkill(int skillLevel) {
    return std::clamp(1 + skillLevel / 2, 1, 11);
}

Game::Game()
    : window(sf::VideoMode::getDesktopMode(), "Chess — The Royal Game",
             sf::Style::Fullscreen)
{
    window.setFramerateLimit(60);
    loadTextures();

    if (!blueFont.loadFromFile("Sprites/Bluefont.fnt", "Sprites/Bluefont.png"))
        std::cerr << "Failed to load Bluefont\n";

    if (!stylishFont.loadFromFile("Sprites/BlackStylishfont.fnt", "Sprites/BlackStylishfont.png"))
        std::cerr << "Failed to load BlackStylishfont\n";

    if (!whiteFont.loadFromFile("Sprites/whitefont.fnt", "Sprites/whitefont.png"))
        std::cerr << "Failed to load whitefont\n";

    menu = new Menu(window, stylishFont, blueFont, textures);
    menu->startFadeIn();
    setupScreen    = new PlayerSetupScreen(window, stylishFont, whiteFont);
    settingsScreen = new SettingsScreen(window, stylishFont, whiteFont);

    ensureDirectoriesExist();
    loadConfig();
}

void Game::loadTextures() {
    std::map<char, std::string> files = {
        {'p', "black-pawn"},    {'n', "black-knight"}, {'b', "black-bishop"},
        {'r', "black-rook"},    {'q', "black-queen"},
        {'k', "30aedb515c31b0cc22a732c61607ed6c1c19a4ba-removebg-preview"},
        {'P', "white-pawn"},    {'N', "white-knight"}, {'B', "white-bishop"},
        {'R', "white-rook"},    {'Q', "white-queen"},
        {'K', "maxresdefault-removebg-preview"}
    };
    for (auto& [c, name] : files) {
        if (!textures[c].loadFromFile("Sprites/" + name + ".png"))
            std::cerr << "Failed: Sprites/" << name << ".png\n";
    }

    if (!whiteTurnTex.loadFromFile("Sprites/whiteturn.png"))
        std::cerr << "Failed: Sprites/whiteturn.png\n";
    if (!blackTurnTex.loadFromFile("Sprites/blackturn.png"))
        std::cerr << "Failed: Sprites/blackturn.png\n";
    
    if (!gameBgTexture.loadFromFile("Sprites/texure-bg.jpg")) {
        std::cerr << "Failed: Sprites/texure-bg.jpg\n";
    } else {
        gameBgSprite.setTexture(gameBgTexture);
        gameBgLoaded = true;
    }
}

void Game::run() {
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        // Cap dt to avoid spiral of death on slow frames
        if (dt > 0.05f) dt = 0.05f;

        processEvents();
        update(dt);
        render();
    }
    delete menu;
    delete setupScreen;
    delete settingsScreen;
}

void Game::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            window.close();

        if (currentState == State::MENU) {
            if (exitConfirmOpen) {
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Escape ||
                        event.key.code == sf::Keyboard::N) {
                        exitConfirmOpen = false;
                    } else if (event.key.code == sf::Keyboard::Y ||
                               event.key.code == sf::Keyboard::Return) {
                        saveConfig();
                        window.close();
                    }
                } else if (event.type == sf::Event::MouseButtonPressed &&
                           event.mouseButton.button == sf::Mouse::Left) {
                    float W = (float)window.getSize().x;
                    float H = (float)window.getSize().y;
                    sf::Vector2f mp = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
                    float cardW = 390.f, cardH = 190.f;
                    float cardX = W / 2.f - cardW / 2.f;
                    float cardY = H / 2.f - cardH / 2.f;
                    float btnW = 120.f, btnH = 40.f, gap = 18.f;
                    float yesX = cardX + cardW / 2.f - btnW - gap / 2.f;
                    float noX = cardX + cardW / 2.f + gap / 2.f;
                    float btnY = cardY + cardH - 62.f;
                    if (sf::FloatRect(yesX, btnY, btnW, btnH).contains(mp)) {
                        saveConfig();
                        window.close();
                    } else if (sf::FloatRect(noX, btnY, btnW, btnH).contains(mp)) {
                        exitConfirmOpen = false;
                    }
                }
                continue;
            }
            menu->handleEvent(event);
        } else if (currentState == State::PLAYER_SETUP) {
            setupScreen->handleEvent(event);
        } else if (currentState == State::SETTINGS) {
            settingsScreen->handleEvent(event);
        } else if (currentState == State::LOAD_GAME) {
            handleLoadGameEvents(event);
        } else if (currentState == State::PROMOTING) {
            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                
                sf::Vector2f mp = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
                float centerX = window.getSize().x / 2.0f;
                float centerY = window.getSize().y / 2.0f;
                float cardW = std::min(120.0f, (float)window.getSize().x * 0.14f);
                float gap = 20.0f;
                float totalW = cardW * 4 + gap * 3;
                float startX = centerX - totalW / 2.0f;

                for (int i = 0; i < 4; ++i) {
                    sf::FloatRect cardRect(startX + i * (cardW + gap), centerY - cardW / 2.0f, cardW, cardW);
                    if (cardRect.contains(mp)) {
                        char promo;
                        if (i == 0) promo = board.isWhiteTurn ? 'Q' : 'q';
                        else if (i == 1) promo = board.isWhiteTurn ? 'R' : 'r';
                        else if (i == 2) promo = board.isWhiteTurn ? 'B' : 'b';
                        else promo = board.isWhiteTurn ? 'N' : 'n';
                        
                        board.promotePiece(promotionCoord.x, promotionCoord.y, promo);
                        
                        // Promotion complete: save timeline!
                        isTimerRunning = true;
                        if (!board.isWhiteTurn) {
                            whiteTime += incrementTime;
                        } else {
                            blackTime += incrementTime;
                        }
                        activeDelayRemaining = delayTime;
                        
                        std::string finalMoveStr = tempPromoMovePrefix + "=" + (char)std::toupper(promo);
                        appendMoveToTimeline(finalMoveStr, makeUCIMove(lastFromCoord.x, lastFromCoord.y,
                                                                       promotionCoord.x, promotionCoord.y, promo));
                        
                        currentState = State::PLAYING;
                        requestAIMoveIfNeeded();
                        break;
                    }
                }
            }
        } else {
            // ─── Shared board geometry ─────────────────────────────────────
            float boardSize = 8.0f * TILE;
            float offsetX   = (window.getSize().x - boardSize) / 2.0f;
            float offsetY   = (window.getSize().y - boardSize) / 2.0f;

            bool isDraw = board.isStalemate(board.isWhiteTurn) ||
                          board.halfmoveClock >= 100 ||
                          checkThreefoldRepetition() ||
                          board.isInsufficientMaterial();

            bool isWhiteKingInCenter = false;
            bool isBlackKingInCenter = false;
            for (int r = 3; r <= 4; ++r) {
                for (int c = 3; c <= 4; ++c) {
                    if (board.board[r][c] == 'K') isWhiteKingInCenter = true;
                    if (board.board[r][c] == 'k') isBlackKingInCenter = true;
                }
            }
            bool kingOfHillWin = (activeVariant == "King of the Hill" && (isWhiteKingInCenter || isBlackKingInCenter));

            bool threeCheckWin = (activeVariant == "3-Check" && (board.checksDeliveredByWhite >= 3 || board.checksDeliveredByBlack >= 3));

            bool gameOver = board.isCheckmate(board.isWhiteTurn) ||
                            isDraw ||
                            (whiteTime <= 0.f && !isUnlimitedTime) ||
                            (blackTime <= 0.f && !isUnlimitedTime) ||
                            kingOfHillWin ||
                            threeCheckWin;

            // Helper: fully commit a confirmed non-promotion move
            // animate=true  → piece slides (click-to-click)
            // animate=false → instant snap (drag-and-drop)
            auto commitMove = [&](int fromR, int fromC, int toR, int toC,
                                  const std::string& lan, char movingPiece, bool animate) {
                // Slide animation (click-to-click only)
                if (animate) {
                    animPiece    = movingPiece;
                    sf::Vector2f fromPos = getSquareScreenPos(fromR, fromC);
                    sf::Vector2f toPos   = getSquareScreenPos(toR, toC);
                    animFrom     = fromPos + sf::Vector2f(TILE / 2.f, TILE / 2.f);
                    animTo       = toPos + sf::Vector2f(TILE / 2.f, TILE / 2.f);
                    animProgress = 0.f;
                    isAnimating  = true;
                } else {
                    isAnimating = false;
                }

                // Set last-move highlight
                lastMoveFrom = { fromR, fromC };
                lastMoveTo   = { toR,   toC   };

                isTimerRunning = true;
                // Add increment to the player who just moved
                if (!board.isWhiteTurn) {
                    whiteTime += incrementTime;
                } else {
                    blackTime += incrementTime;
                }
                activeDelayRemaining = delayTime;

                appendMoveToTimeline(lan, makeUCIMove(fromR, fromC, toR, toC));
                requestAIMoveIfNeeded();
            };

            // ─── Mouse Button Pressed ──────────────────────────────────────
            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {

                sf::Vector2f mp = window.mapPixelToCoords(
                    {event.mouseButton.x, event.mouseButton.y});

                // ─── Priority 0: End-game modal intercept ──────────────
                if (isGameFinished && endGamePulse >= 2.0f) {
                    float mW = (float)window.getSize().x;
                    float mH = (float)window.getSize().y;

                    if (!isReviewing) {
                        // Modal is visible — handle its 4 action buttons
                        float cardW2 = 560.f, cardH2 = 480.f;
                        float cardX2 = mW / 2.f - cardW2 / 2.f;
                        float cardY2 = mH / 2.f - cardH2 / 2.f;
                        float btnW2  = 110.f, btnH2 = 42.f, btnGap2 = 14.f;
                        float btnRowX2 = cardX2 + (cardW2 - 4.f*btnW2 - 3.f*btnGap2) / 2.f;
                        float btnRowY2 = cardY2 + cardH2 - 68.f;

                        for (int b = 0; b < 4; ++b) {
                            float bX2 = btnRowX2 + b * (btnW2 + btnGap2);
                            if (sf::FloatRect(bX2, btnRowY2, btnW2, btnH2).contains(mp)) {
                                if (b == 0) {  // RETRY
                                    board = ChessBoard();
                                    stateHistory.clear();
                                    moveList.clear();
                                    uciMoveHistory.clear();
                                    stateHistory.push_back(board);
                                    currentMoveIndex = 0;
                                    aiMovePending = false;
                                    whiteTime        = initialTimeLimit;
                                    blackTime        = initialTimeLimit;
                                    activeDelayRemaining = delayTime;
                                    isTimerRunning   = false;
                                    isGameFinished   = false;
                                    isReviewing      = false;
                                    endGamePulse     = 0.f;
                                    lastMoveFrom = lastMoveTo = { -1, -1 };
                                    selectedSquare = { -1, -1 };
                                    isDragging     = false;
                                    time_t t2 = time(0);
                                    struct tm* now2 = localtime(&t2);
                                    char fname2[80];
                                    if (now2) strftime(fname2, sizeof(fname2), "data/saves/match_%Y%m%d_%H%M%S.pgn", now2);
                                    else       sprintf(fname2, "data/saves/match_retry.pgn");
                                    currentPGNPath = fname2;
                                    saveCurrentGamePGN();
                                } else if (b == 1) {  // REVIEW
                                    isReviewing = true;
                                } else if (b == 2) {  // MAIN MENU
                                    isGameFinished = false;
                                    isReviewing    = false;
                                    endGamePulse   = 0.f;
                                    saveCurrentGamePGN();
                                    isDragging     = false;
                                    selectedSquare = { -1, -1 };
                                    currentState   = State::MENU;
                                    menu->startFadeIn();
                                } else {              // EXIT GAME
                                    saveConfig();
                                    window.close();
                                }
                                break;
                            }
                        }
                        // Consume this click — modal is on top of everything
                        continue;
                    } else {
                        // Reviewing — only intercept the floating RESULT button
                        float rbW = 152.f, rbH = 40.f;
                        float rbX = mW - rbW - 22.f;
                        float rbY = mH - rbH - 22.f;
                        if (sf::FloatRect(rbX, rbY, rbW, rbH).contains(mp)) {
                            isReviewing = false;
                            continue;
                        }
                        // Otherwise fall through — allow undo/redo deck clicks
                    }
                }

                // Priority 1: Animated session menu
                float gearX = 38.f, gearY = 32.f, gearSz = 42.f;
                bool hitGear = sf::FloatRect(gearX, gearY, gearSz, gearSz).contains(mp);
                int hitSessionOption = -1;
                if (!hitGear && sessionMenuAnim > 0.35f) {
                    float optW = 98.f, optH = 34.f, optGap = 8.f;
                    float optX = gearX + gearSz + 14.f;
                    float optY = gearY + (gearSz - optH) / 2.f;
                    float ease = sessionMenuAnim * sessionMenuAnim * (3.f - 2.f * sessionMenuAnim);
                    float closedX = gearX + gearSz * 0.35f;
                    for (int i = 0; i < 3; ++i) {
                        float openX = optX + i * (optW + optGap);
                        float visualX = closedX + (openX - closedX) * ease;
                        if (sf::FloatRect(visualX, optY, optW, optH).contains(mp)) {
                            hitSessionOption = i;
                            break;
                        }
                    }
                }

                if (hitGear) {
                    sessionMenuOpen = !sessionMenuOpen;
                    isDragging = false;
                    selectedSquare = { -1, -1 };
                    selectedPocketPiece = '\0';

                } else if (hitSessionOption != -1) {
                    sessionMenuOpen = false;
                    isDragging = false;
                    selectedSquare = { -1, -1 };
                    selectedPocketPiece = '\0';
                    if (hitSessionOption == 0) {
                        previousState = State::PLAYING;
                        startTransition(State::SETTINGS);
                    } else if (hitSessionOption == 1) {
                        aiMovePending = false;
                        saveCurrentGamePGN();
                        currentState = State::MENU;
                        menu->startFadeIn();
                    } else {
                        saveConfig();
                        window.close();
                    }

                } else if (handleMoveHistoryClick(mp)) {
                    isDragging = false;
                    selectedSquare = { -1, -1 };
                    selectedPocketPiece = '\0';

                } else if (!gameOver && isHumanTurn()) {
                    // Check pocket clicks first!
                    bool clickedPocket = false;
                    if (activeVariant == "Crazyhouse") {
                        bool isWhite = board.isWhiteTurn;
                        const char* pieces = isWhite ? "PNBRQ" : "pnbrq";
                        auto& pocket = isWhite ? board.whitePocket : board.blackPocket;
                        
                        for (int i = 0; i < 5; ++i) {
                            sf::FloatRect rect = getPocketSlotRect(isWhite, i, offsetX, offsetY, boardSize);
                            if (rect.contains(mp)) {
                                char p = pieces[i];
                                int count = pocket.count(p) ? pocket.at(p) : 0;
                                if (count > 0) {
                                    clickedPocket = true;
                                    selectedSquare = { -1, -1 }; // clear board piece selection
                                    
                                    if (selectedPocketPiece == p) {
                                        selectedPocketPiece = '\0';
                                        isDragging = false;
                                    } else {
                                        selectedPocketPiece = p;
                                        isDragging = true;
                                        dragCurrentPos = mp;
                                    }
                                }
                                break;
                            }
                        }
                    }

                    if (!clickedPocket) {
                        int col = (int)((event.mouseButton.x - offsetX) / TILE);
                        int row = (int)((event.mouseButton.y - offsetY) / TILE);
                        bool onBoard = col >= 0 && col < 8 && row >= 0 && row < 8;
                        if (onBoard && isBoardFlipped()) {
                            row = 7 - row;
                            col = 7 - col;
                        }

                        if (onBoard) {
                            if (selectedPocketPiece != '\0') {
                                // Try to drop
                                if (board.isValidDrop(selectedPocketPiece, row, col)) {
                                    std::string dropNotation = std::string(1, std::toupper(selectedPocketPiece)) + "@" + (char)(col + 'a') + std::to_string(8 - row);
                                    board.dropPiece(selectedPocketPiece, row, col);
                                    
                                    isTimerRunning = true;
                                    if (!board.isWhiteTurn) {
                                        whiteTime += incrementTime;
                                    } else {
                                        blackTime += incrementTime;
                                    }
                                    activeDelayRemaining = delayTime;
                                    
                                    if (currentMoveIndex < (int)stateHistory.size() - 1) {
                                        stateHistory.erase(stateHistory.begin() + currentMoveIndex + 1, stateHistory.end());
                                        moveList.erase(moveList.begin() + currentMoveIndex, moveList.end());
                                    }
                                    moveList.push_back(dropNotation);
                                    stateHistory.push_back(board);
                                    currentMoveIndex++;
                                    saveCurrentGamePGN();
                                    
                                    selectedPocketPiece = '\0';
                                    isDragging = false;
                                } else {
                                    selectedPocketPiece = '\0';
                                    isDragging = false;
                                }
                            } else {
                                char piece = board.board[row][col];
                                bool isOwn = piece != '.' &&
                                    (board.isWhiteTurn ? (bool)std::isupper(piece)
                                                      : (bool)std::islower(piece));

                                if (selectedSquare.x != -1) {
                                    // — Second action with a piece already selected —
                                    if (board.isValidMove(selectedSquare.x, selectedSquare.y, row, col)) {
                                        // Execute click-to-click move
                                        int fromR = selectedSquare.x, fromC = selectedSquare.y;
                                        char movingPiece = board.board[fromR][fromC];
                                        std::string lan  = board.getLANMove(fromR, fromC, row, col);
                                        if (board.movePiece(fromR, fromC, row, col)) {
                                            // Pawn promotion
                                            tempPromoMovePrefix = lan;
                                            lastFromCoord = { fromR, fromC };
                                            lastMoveFrom = { fromR, fromC };
                                            lastMoveTo   = { row, col };
                                            currentState = State::PROMOTING;
                                            promotionCoord = { row, col };
                                        } else {
                                            commitMove(fromR, fromC, row, col, lan, movingPiece, true); // animate
                                        }
                                        selectedSquare = { -1, -1 };
                                        isDragging     = false;
                                    } else if (isOwn) {
                                        // Reselect a different own piece
                                        selectedSquare   = { row, col };
                                        dragSourceSquare = { row, col };
                                        isDragging       = true;
                                        dragCurrentPos   = { (float)event.mouseButton.x,
                                                             (float)event.mouseButton.y };
                                    } else {
                                        selectedSquare = { -1, -1 };
                                        isDragging     = false;
                                    }
                                } else {
                                    // — First selection —
                                    if (isOwn) {
                                        selectedSquare   = { row, col };
                                        dragSourceSquare = { row, col };
                                        isDragging       = true;
                                        dragCurrentPos   = { (float)event.mouseButton.x,
                                                             (float)event.mouseButton.y };
                                    }
                                }
                            }
                        } else {
                            selectedSquare = { -1, -1 };
                            selectedPocketPiece = '\0';
                            isDragging     = false;
                        }
                    }
                } else if (!isHumanTurn()) {
                    selectedSquare = { -1, -1 };
                    selectedPocketPiece = '\0';
                    isDragging = false;
                }
            }

            // ─── Mouse Moved (drag tracking) ──────────────────────────────
            if (event.type == sf::Event::MouseMoved && isDragging && isHumanTurn()) {
                dragCurrentPos = { (float)event.mouseMove.x, (float)event.mouseMove.y };
            }

            // ─── Mouse Button Released (drag drop) ────────────────────────
            if (event.type == sf::Event::MouseButtonReleased &&
                event.mouseButton.button == sf::Mouse::Left && isDragging) {

                isDragging = false;
                if (!isHumanTurn()) {
                    selectedSquare = { -1, -1 };
                    selectedPocketPiece = '\0';
                    continue;
                }
                
                if (selectedPocketPiece != '\0') {
                    // Dropping a pocket piece!
                    float offsetX2 = (window.getSize().x - 8.0f*TILE) / 2.0f;
                    float offsetY2 = (window.getSize().y - 8.0f*TILE) / 2.0f;
                    int col = (int)((event.mouseButton.x - offsetX2) / TILE);
                    int row = (int)((event.mouseButton.y - offsetY2) / TILE);
                    bool onBoard = col >= 0 && col < 8 && row >= 0 && row < 8;
                    if (onBoard && isBoardFlipped()) {
                        row = 7 - row;
                        col = 7 - col;
                    }
                    
                    // Check if released on the same pocket slot
                    bool releasedOnSamePocket = false;
                    bool isWhite = board.isWhiteTurn;
                    const char* pieces = isWhite ? "PNBRQ" : "pnbrq";
                    for (int i = 0; i < 5; ++i) {
                        sf::FloatRect rect = getPocketSlotRect(isWhite, i, offsetX2, offsetY2, 8.0f*TILE);
                        if (rect.contains({(float)event.mouseButton.x, (float)event.mouseButton.y}) && pieces[i] == selectedPocketPiece) {
                            releasedOnSamePocket = true;
                            break;
                        }
                    }
                    
                    if (onBoard && !gameOver && board.isValidDrop(selectedPocketPiece, row, col)) {
                        std::string dropNotation = std::string(1, std::toupper(selectedPocketPiece)) + "@" + (char)(col + 'a') + std::to_string(8 - row);
                        
                        board.dropPiece(selectedPocketPiece, row, col);
                        
                        isTimerRunning = true;
                        if (!board.isWhiteTurn) {
                            whiteTime += incrementTime;
                        } else {
                            blackTime += incrementTime;
                        }
                        activeDelayRemaining = delayTime;
                        
                        if (currentMoveIndex < (int)stateHistory.size() - 1) {
                            stateHistory.erase(stateHistory.begin() + currentMoveIndex + 1, stateHistory.end());
                            moveList.erase(moveList.begin() + currentMoveIndex, moveList.end());
                        }
                        moveList.push_back(dropNotation);
                        stateHistory.push_back(board);
                        currentMoveIndex++;
                        saveCurrentGamePGN();
                        
                        selectedPocketPiece = '\0';
                    } else if (!releasedOnSamePocket) {
                        // Cancel selection if dropped on invalid board square or outside board
                        selectedPocketPiece = '\0';
                    }
                } else {
                    float offsetX2 = (window.getSize().x - 8.0f*TILE) / 2.0f;
                    float offsetY2 = (window.getSize().y - 8.0f*TILE) / 2.0f;
                    int col = (int)((event.mouseButton.x - offsetX2) / TILE);
                    int row = (int)((event.mouseButton.y - offsetY2) / TILE);
                    bool onBoard = col >= 0 && col < 8 && row >= 0 && row < 8;
                    if (onBoard && isBoardFlipped()) {
                        row = 7 - row;
                        col = 7 - col;
                    }

                    bool droppedOnSelf = onBoard &&
                        row == dragSourceSquare.x && col == dragSourceSquare.y;

                    if (!droppedOnSelf && onBoard && !gameOver &&
                        board.isValidMove(dragSourceSquare.x, dragSourceSquare.y, row, col)) {
                        int fromR = dragSourceSquare.x, fromC = dragSourceSquare.y;
                        char movingPiece = board.board[fromR][fromC];
                        std::string lan  = board.getLANMove(fromR, fromC, row, col);
                        if (board.movePiece(fromR, fromC, row, col)) {
                            // Pawn promotion
                            tempPromoMovePrefix = lan;
                            lastFromCoord = { fromR, fromC };
                            lastMoveFrom = { fromR, fromC };
                            lastMoveTo   = { row, col };
                            currentState = State::PROMOTING;
                            promotionCoord = { row, col };
                        } else {
                            commitMove(fromR, fromC, row, col, lan, movingPiece, false); // no animate for drag
                        }
                        selectedSquare = { -1, -1 };
                    } else if (!droppedOnSelf) {
                        // Dropped on invalid square — cancel selection
                        selectedSquare = { -1, -1 };
                    }
                }
            }

            // ─── Escape Key ───────────────────────────────────────────────
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape) {
                previousState = State::PLAYING;
                startTransition(State::SETTINGS);
                selectedSquare   = { -1, -1 };
                selectedPocketPiece = '\0';
                isDragging       = false;
                sessionMenuOpen  = false;
            }
        }
    }
}

void Game::update(float dt) {
    uiPulseTime += dt;

    // Advance slide animation
    if (isAnimating) {
        animProgress += dt / animDuration;
        if (animProgress >= 1.f) {
            animProgress = 1.f;
            isAnimating  = false;
        }
    }

    float menuTarget = sessionMenuOpen ? 1.f : 0.f;
    sessionMenuAnim += (menuTarget - sessionMenuAnim) * std::min(1.f, dt * 10.f);
    if (std::abs(sessionMenuAnim - menuTarget) < 0.01f) sessionMenuAnim = menuTarget;

    float exitTarget = exitConfirmOpen ? 1.f : 0.f;
    exitConfirmAnim = ui::smoothToward(exitConfirmAnim, exitTarget, dt, 12.f);
    if (std::abs(exitConfirmAnim - exitTarget) < 0.01f) exitConfirmAnim = exitTarget;

    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    float cardW = 390.f, cardH = 190.f;
    float cardX = W / 2.f - cardW / 2.f;
    float cardY = H / 2.f - cardH / 2.f;
    float btnW = 120.f, btnH = 40.f, gap = 18.f;
    float yesX = cardX + cardW / 2.f - btnW - gap / 2.f;
    float noX = cardX + cardW / 2.f + gap / 2.f;
    float btnY = cardY + cardH - 62.f;
    bool yesHover = exitConfirmOpen && sf::FloatRect(yesX, btnY, btnW, btnH).contains(mp);
    bool noHover = exitConfirmOpen && sf::FloatRect(noX, btnY, btnW, btnH).contains(mp);
    exitYesHover = ui::smoothToward(exitYesHover, yesHover ? 1.f : 0.f, dt, 12.f);
    exitNoHover = ui::smoothToward(exitNoHover, noHover ? 1.f : 0.f, dt, 12.f);

    // Handle screen transition
    updateTransition(dt);

    if (currentState == State::MENU) {
        menu->update(dt);
        MenuAction action = menu->getAction();
        if (action != MenuAction::None) {
            menu->clearAction();
            if (action == MenuAction::Play) {
                pendingVsAIStart = false;
                setupScreen->setAIMode(false);
                startTransition(State::PLAYER_SETUP);
            } else if (action == MenuAction::PlayAI) {
                pendingVsAIStart = true;
                setupScreen->setAIMode(true);
                startTransition(State::PLAYER_SETUP);
            } else if (action == MenuAction::LoadGame) {
                scanSavedMatches();
                selectedMatchIndex = 0;
                startTransition(State::LOAD_GAME);
            } else if (action == MenuAction::Settings) {
                previousState = State::MENU;
                startTransition(State::SETTINGS);
            } else if (action == MenuAction::Exit) {
                exitConfirmOpen = true;
            }
        }
    } else if (currentState == State::PLAYER_SETUP) {
        setupScreen->update(dt);
        SetupAction sa = setupScreen->getAction();
        if (sa == SetupAction::StartMatch) {
            setupScreen->clearAction();
            isVsAI = pendingVsAIStart;
            bool randomColor = setupScreen->getColorAssignment() == "Random";
            static std::mt19937 colorRng(std::random_device{}());
            bool randomChoice = randomColor && std::uniform_int_distribution<int>(0, 1)(colorRng) == 0;
            if (isVsAI) {
                bool humanWantsBlack = setupScreen->getColorAssignment() == "Black";
                bool humanPlaysWhite = setupScreen->getColorAssignment() == "White" || (randomColor && randomChoice);
                aiPlaysWhite = humanWantsBlack || (randomColor && !humanPlaysWhite);
                player1Name = aiPlaysWhite ? "Stockfish" : setupScreen->getPlayer1Name();
                player2Name = aiPlaysWhite ? setupScreen->getPlayer1Name() : "Stockfish";
            } else {
                bool swapPlayers = randomChoice;
                player1Name = swapPlayers ? setupScreen->getPlayer2Name() : setupScreen->getPlayer1Name();
                player2Name = swapPlayers ? setupScreen->getPlayer1Name() : setupScreen->getPlayer2Name();
                aiPlaysWhite = false;
            }
            board = ChessBoard();

            activeVariant = isVsAI ? "Standard" : setupScreen->getGameVariant();
            activePerspective = setupScreen->getPerspective();
            
            if (!isVsAI && activeVariant == "Chess 960") {
                board.setupChess960();
            }

            // Parse time controls from setup screen
            std::string baseTimeStr = setupScreen->getBaseTime();
            if (baseTimeStr == "Unlimited") {
                isUnlimitedTime = true;
                initialTimeLimit = 0.f;
            } else {
                isUnlimitedTime = false;
                initialTimeLimit = parseBaseTime(baseTimeStr);
            }
            incrementTime = parseSeconds(setupScreen->getIncrement());
            delayTime = parseSeconds(setupScreen->getDelay());
            aiSkillLevel = parseAISkillLevel(setupScreen->getAIDifficulty());
            aiSearchDepth = getDepthForAISkill(aiSkillLevel);
            aiMovePending = false;
            aiMoveDelayRemaining = 0.f;

            // Initialize Undo/Redo tracking
            stateHistory.clear();
            moveList.clear();
            uciMoveHistory.clear();
            stateHistory.push_back(board);
            currentMoveIndex = 0;

            // Initialize player clocks
            whiteTime = initialTimeLimit;
            blackTime = initialTimeLimit;
            activeDelayRemaining = delayTime;
            isTimerRunning = false; // Start ticking on first move
            isGameFinished = false;
            isReviewing    = false;
            endGamePulse   = 0.f;

            // Initialize PGN filename
            time_t t = time(0);
            struct tm* now = localtime(&t);
            char filename[80];
            if (now) {
                strftime(filename, sizeof(filename), "data/saves/match_%Y%m%d_%H%M%S.pgn", now);
            } else {
                sprintf(filename, "data/saves/match_game.pgn");
            }
            currentPGNPath = filename;
            saveCurrentGamePGN();

            startTransition(State::PLAYING);
        } else if (sa == SetupAction::Back) {
            setupScreen->clearAction();
            startTransition(State::MENU);
        }
    } else if (currentState == State::SETTINGS) {
        settingsScreen->update(dt);
        if (settingsScreen->getAction() == SettingsAction::Back) {
            settingsScreen->clearAction();
            // Return to whichever screen opened Settings
            if (previousState == State::MENU) menu->startFadeIn();
            startTransition(previousState);
        }
    } else if (currentState == State::LOAD_GAME) {
        // Simple screen, no specific state update needed here
    } else {
        if (currentState == State::PLAYING) {
            requestAIMoveIfNeeded();
            if (aiMovePending) {
                aiMoveDelayRemaining -= dt;
                if (aiMoveDelayRemaining <= 0.f) {
                    aiMovePending = false;
                    executePendingAIMove();
                }
            }
        }

        bool inCheckmate = board.isCheckmate(board.isWhiteTurn);
        bool inStalemate = board.isStalemate(board.isWhiteTurn);
        bool is50Moves = (board.halfmoveClock >= 100);
        bool isRepetition = checkThreefoldRepetition();
        bool isInsufficient = board.isInsufficientMaterial();
        bool isDraw = inStalemate || is50Moves || isRepetition || isInsufficient;
        bool flagFallWhite = (whiteTime <= 0.f) && !isUnlimitedTime;
        bool flagFallBlack = (blackTime <= 0.f) && !isUnlimitedTime;
        
        bool isWhiteKingInCenter = false;
        bool isBlackKingInCenter = false;
        for (int r = 3; r <= 4; ++r) {
            for (int c = 3; c <= 4; ++c) {
                if (board.board[r][c] == 'K') isWhiteKingInCenter = true;
                if (board.board[r][c] == 'k') isBlackKingInCenter = true;
            }
        }
        bool kingOfHillWhiteWin = (activeVariant == "King of the Hill" && isWhiteKingInCenter);
        bool kingOfHillBlackWin = (activeVariant == "King of the Hill" && isBlackKingInCenter);
        bool kingOfHillWin = kingOfHillWhiteWin || kingOfHillBlackWin;

        bool threeCheckWhiteWin = (activeVariant == "3-Check" && board.checksDeliveredByWhite >= 3);
        bool threeCheckBlackWin = (activeVariant == "3-Check" && board.checksDeliveredByBlack >= 3);
        bool threeCheckWin = threeCheckWhiteWin || threeCheckBlackWin;

        bool gameOver = inCheckmate || isDraw || flagFallWhite || flagFallBlack || kingOfHillWin || threeCheckWin;
        
        // 1. Clock Decrementing
        if (isTimerRunning && !gameOver && !isUnlimitedTime) {
            if (activeDelayRemaining > 0.f) {
                activeDelayRemaining -= dt;
                if (activeDelayRemaining < 0.f) {
                    float overflow = -activeDelayRemaining;
                    if (board.isWhiteTurn) {
                        whiteTime -= overflow;
                    } else {
                        blackTime -= overflow;
                    }
                    activeDelayRemaining = 0.f;
                }
            } else {
                if (board.isWhiteTurn) {
                    whiteTime -= dt;
                } else {
                    blackTime -= dt;
                }
            }
            if (whiteTime <= 0.f) {
                whiteTime = 0.f;
                isTimerRunning = false;
            }
            if (blackTime <= 0.f) {
                blackTime = 0.f;
                isTimerRunning = false;
            }
        }
        
        // 2. Stats Accumulation
        if (gameOver && !isGameFinished) {
            isGameFinished = true;
            endGamePulse   = 0.f;  // start modal animation from zero
            statTotalGames++;
            
            if (inCheckmate) {
                if (board.isWhiteTurn) statBlackWins++;
                else statWhiteWins++;
            } else if (kingOfHillWin) {
                if (kingOfHillWhiteWin) statWhiteWins++;
                else statBlackWins++;
            } else if (threeCheckWin) {
                if (threeCheckWhiteWin) statWhiteWins++;
                else statBlackWins++;
            } else if (isDraw) {
                statDraws++;
            } else if (flagFallWhite) {
                statBlackWins++;
            } else if (flagFallBlack) {
                statWhiteWins++;
            }
            
            saveConfig();
            saveCurrentGamePGN();
        }
        
        // 3. Advance end-game pulse timer
        if (isGameFinished) endGamePulse += dt;

        // 4. Title Updates
        std::string title = "Chess — ";
        if (inCheckmate) {
            title += std::string(board.isWhiteTurn ? "Black" : "White") + " Wins! (Checkmate)";
        } else if (kingOfHillWin) {
            title += std::string(kingOfHillWhiteWin ? "White" : "Black") + " Wins! (King of the Hill)";
        } else if (threeCheckWin) {
            title += std::string(threeCheckWhiteWin ? "White" : "Black") + " Wins! (3-Check)";
        } else if (inStalemate) {
            title += "Draw (Stalemate)";
        } else if (is50Moves) {
            title += "Draw (50-move rule)";
        } else if (isRepetition) {
            title += "Draw (Threefold repetition)";
        } else if (isInsufficient) {
            title += "Draw (Insufficient material)";
        } else if (whiteTime <= 0.f && !isUnlimitedTime) {
            title += "Black Wins! (White ran out of time)";
        } else if (blackTime <= 0.f && !isUnlimitedTime) {
            title += "White Wins! (Black ran out of time)";
        } else {
            title += board.isWhiteTurn ? "White's Turn" : "Black's Turn";
            if (board.isKingInCheck(board.isWhiteTurn)) {
                title += " — CHECK!";
            }
            if (activeVariant == "3-Check") {
                title += " (" + std::to_string(board.checksDeliveredByWhite) + "/" + std::to_string(board.checksDeliveredByBlack) + " checks)";
            }
        }
        window.setTitle(title);
    }
}

void Game::render() {
    if (currentState == State::MENU) {
        menu->draw();
        renderExitConfirmation();
    } else if (currentState == State::PLAYER_SETUP) {
        setupScreen->draw();
    } else if (currentState == State::SETTINGS) {
        settingsScreen->draw();
    } else if (currentState == State::LOAD_GAME) {
        renderLoadGameScreen();
    } else {
        renderGame();
        if (currentState == State::PROMOTING) {
            // Draw dimmed overlay
            sf::RectangleShape overlay(sf::Vector2f((float)window.getSize().x, (float)window.getSize().y));
            overlay.setFillColor(sf::Color(0, 0, 0, 180));
            window.draw(overlay);

            // Draw Promotion UI
            float centerX = window.getSize().x / 2.0f;
            float centerY = window.getSize().y / 2.0f;
            float cardW = std::min(120.0f, (float)window.getSize().x * 0.14f);
            float gap = 20.0f;
            float totalW = cardW * 4 + gap * 3;
            float startX = centerX - totalW / 2.0f;

            std::string labels[] = {"QUEEN", "ROOK", "BISHOP", "KNIGHT"};
            char pieces[] = {board.isWhiteTurn ? 'Q' : 'q', 
                             board.isWhiteTurn ? 'R' : 'r', 
                             board.isWhiteTurn ? 'B' : 'b', 
                             board.isWhiteTurn ? 'N' : 'n'};

            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            sf::Vector2f mp = window.mapPixelToCoords(mousePos);

            for (int i = 0; i < 4; ++i) {
                sf::FloatRect cardRect(startX + i * (cardW + gap), centerY - cardW / 2.0f, cardW, cardW);
                bool hovered = cardRect.contains(mp);

                ui::drawRoundedRect(window, {cardRect.left + 5.f, cardRect.top + 6.f, cardRect.width, cardRect.height}, 7.f,
                                    sf::Color(0, 0, 0, 85));
                ui::drawRoundedPanel(window, cardRect, 7.f,
                                     hovered ? sf::Color(200, 170, 40) : sf::Color(34, 30, 22, 245),
                                     hovered ? sf::Color(255, 230, 150) : sf::Color(220, 190, 100),
                                     hovered ? 2.0f : 1.4f);

                sf::Sprite s(textures[pieces[i]]);
                float sScale = (cardW * 0.7f) / textures[pieces[i]].getSize().x;
                s.setScale(sScale, sScale);
                s.setOrigin(textures[pieces[i]].getSize().x / 2.0f, textures[pieces[i]].getSize().y / 2.0f);
                s.setPosition(cardRect.left + cardW/2.0f, cardRect.top + cardW/2.0f - 10.0f);
                window.draw(s);

                float labelScale = 0.15f;
                float lw = blueFont.getTextWidth(labels[i], labelScale);
                blueFont.drawText(window, labels[i], sf::Vector2f(cardRect.left + cardW/2.0f - lw/2.0f, cardRect.top + cardW - 25.0f), labelScale, sf::Color(200, 180, 140));
            }

            std::string msg = "SELECT PROMOTION";
            float mw = whiteFont.getTextWidth(msg, 0.28f);
            whiteFont.drawText(window, msg, sf::Vector2f(centerX - mw / 2.0f, centerY - cardW - 40.0f), 0.28f, sf::Color(220, 190, 100));
        }

        // ── End-game overlay modal (drawn on top of board & promotion UI) ───
        if (isGameFinished) {
            renderEndGameModal();
        }
    }
    // Draw transition fade overlay on top of everything
    drawTransitionOverlay();
    window.display();
}

void Game::renderExitConfirmation() {
    if (exitConfirmAnim <= 0.01f) return;

    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    sf::Uint8 alpha = (sf::Uint8)std::min(190.f, exitConfirmAnim * 190.f);

    sf::RectangleShape veil({W, H});
    veil.setFillColor(sf::Color(6, 6, 10, alpha));
    window.draw(veil);

    float cardW = 390.f, cardH = 190.f;
    float cardX = W / 2.f - cardW / 2.f;
    float cardY = H / 2.f - cardH / 2.f;
    sf::Uint8 panelAlpha = (sf::Uint8)(235 * exitConfirmAnim);
    sf::Uint8 lineAlpha = (sf::Uint8)(210 * exitConfirmAnim);

    ui::drawRoundedRect(window, {cardX + 7.f, cardY + 9.f, cardW, cardH}, 8.f,
                        sf::Color(0, 0, 0, (sf::Uint8)(90 * exitConfirmAnim)));
    ui::drawRoundedPanel(window, {cardX, cardY, cardW, cardH}, 8.f,
                         sf::Color(20, 20, 26, panelAlpha),
                         sf::Color(130, 110, 50, lineAlpha),
                         1.4f);

    std::string title = "DO YOU WANT TO EXIT?";
    float titleScale = 0.22f;
    float titleW = whiteFont.getTextWidth(title, titleScale);
    whiteFont.drawText(window, title,
                       {cardX + cardW / 2.f - titleW / 2.f, cardY + 42.f},
                       titleScale,
                       sf::Color(230, 220, 190, (sf::Uint8)(255 * exitConfirmAnim)));

    std::string sub = "Unsaved preferences will be written before closing.";
    float subScale = 0.105f;
    float subW = whiteFont.getTextWidth(sub, subScale);
    whiteFont.drawText(window, sub,
                       {cardX + cardW / 2.f - subW / 2.f, cardY + 84.f},
                       subScale,
                       sf::Color(135, 135, 150, (sf::Uint8)(220 * exitConfirmAnim)));

    float btnW = 120.f, btnH = 40.f, gap = 18.f;
    float yesX = cardX + cardW / 2.f - btnW - gap / 2.f;
    float noX = cardX + cardW / 2.f + gap / 2.f;
    float btnY = cardY + cardH - 62.f;

    auto drawChoice = [&](float x, const std::string& label, float hover, bool danger) {
        sf::Color baseFill = danger ? sf::Color(62, 28, 30, panelAlpha)
                                    : sf::Color(28, 28, 36, panelAlpha);
        sf::Color hoverFill = danger ? sf::Color(150, 48, 52, panelAlpha)
                                     : sf::Color(173, 146, 29, panelAlpha);
        sf::Color baseLine = danger ? sf::Color(130, 55, 58, lineAlpha)
                                    : sf::Color(82, 82, 94, lineAlpha);
        sf::Color hoverLine = danger ? sf::Color(215, 85, 88, lineAlpha)
                                     : sf::Color(220, 195, 90, lineAlpha);
        ui::drawRoundedPanel(window, {x, btnY, btnW, btnH}, 5.f,
                             ui::mixColor(baseFill, hoverFill, hover),
                             ui::mixColor(baseLine, hoverLine, hover),
                             1.2f);

        float sc = 0.15f;
        float tw = whiteFont.getTextWidth(label, sc);
        sf::Color normalText = danger ? sf::Color(235, 205, 205, (sf::Uint8)(245 * exitConfirmAnim))
                                      : sf::Color(220, 220, 232, (sf::Uint8)(245 * exitConfirmAnim));
        sf::Color hoverText = danger ? sf::Color(255, 235, 235, (sf::Uint8)(255 * exitConfirmAnim))
                                     : sf::Color(14, 14, 18, (sf::Uint8)(255 * exitConfirmAnim));
        whiteFont.drawText(window, label,
                           {x + btnW / 2.f - tw / 2.f, btnY + btnH / 2.f - 7.f},
                           sc,
                           ui::mixColor(normalText, hoverText, hover));
    };

    drawChoice(yesX, "YES", exitYesHover, true);
    drawChoice(noX, "NO", exitNoHover, false);
}

void Game::renderGame() {
    float windowW = (float)window.getSize().x;
    float windowH = (float)window.getSize().y;
    float boardSize = 8.0f * TILE;
    float offsetX = (windowW - boardSize) / 2.0f;
    float offsetY = (windowH - boardSize) / 2.0f;

    // Draw background image
    if (gameBgLoaded) {
        float sx = windowW / gameBgTexture.getSize().x;
        float sy = windowH / gameBgTexture.getSize().y;
        // Keep aspect ratio by using max scale, or just stretch if it's okay. Stretching is usually fine for subtle BGs.
        gameBgSprite.setScale(sx, sy);
        window.draw(gameBgSprite);

        // Subtle dark overlay to ensure readability and minimalist calm vibe
        sf::RectangleShape bgOverlay(sf::Vector2f(windowW, windowH));
        bgOverlay.setFillColor(sf::Color(15, 15, 20, 160));
        window.draw(bgOverlay);
    } else {
        window.clear(sf::Color(30, 30, 30));
    }

    // Draw minimalist board frame
    float framePadding = 30.0f;
    float frameSize = boardSize + framePadding * 2.0f;
    
    // Soft drop shadow for frame
    sf::RectangleShape shadow(sf::Vector2f(frameSize, frameSize));
    shadow.setPosition(offsetX - framePadding + 10.0f, offsetY - framePadding + 15.0f);
    shadow.setFillColor(sf::Color(0, 0, 0, 80));
    window.draw(shadow);

    // Main clean frame
    sf::RectangleShape frame(sf::Vector2f(frameSize, frameSize));
    frame.setPosition(offsetX - framePadding, offsetY - framePadding);
    frame.setFillColor(sf::Color(25, 25, 30)); // Dark sleek border
    frame.setOutlineThickness(1.5f);
    frame.setOutlineColor(sf::Color(80, 80, 90));
    window.draw(frame);

    // Coordinates (A-H, 1-8) embedded in frame
    bool flipped = isBoardFlipped();
    std::string cols = "abcdefgh";
    for (int i = 0; i < 8; ++i) {
        // Column letters (bottom)
        int displayC = flipped ? 7 - i : i;
        std::string letter(1, std::toupper(cols[displayC]));
        float scale = 0.14f;
        float lw = whiteFont.getTextWidth(letter, scale);
        // Bottom edge
        whiteFont.drawText(window, letter, sf::Vector2f(offsetX + i * TILE + TILE / 2.0f - lw / 2.0f, offsetY + boardSize + framePadding / 2.0f - 7.0f), scale, sf::Color(180, 180, 190));
        
        // Row numbers (left)
        std::string num = std::to_string(flipped ? i + 1 : 8 - i);
        float nw = whiteFont.getTextWidth(num, scale);
        // Left edge
        whiteFont.drawText(window, num, sf::Vector2f(offsetX - framePadding / 2.0f - nw / 2.0f, offsetY + i * TILE + TILE / 2.0f - 7.0f), scale, sf::Color(180, 180, 190));
    }

    // Pre-compute king-in-check info once (avoid 64 redundant calls inside loop)
    bool   kingInCheck  = board.isKingInCheck(board.isWhiteTurn);
    char   checkedKChar = board.isWhiteTurn ? 'K' : 'k';
    int    ckRow = -1, ckCol = -1;
    if (kingInCheck) {
        for (int r = 0; r < 8 && ckRow == -1; ++r)
            for (int c = 0; c < 8 && ckRow == -1; ++c)
                if (board.board[r][c] == checkedKChar) { ckRow = r; ckCol = c; }
    }

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            sf::Vector2f screenPos = getSquareScreenPos(row, col);
            float drawX = screenPos.x;
            float drawY = screenPos.y;

            // ── Base square colour ──────────────────────────────────────
            sf::RectangleShape sq(sf::Vector2f((float)TILE, (float)TILE));
            sq.setPosition(drawX, drawY);
            if (selectedSquare.x == row && selectedSquare.y == col)
                sq.setFillColor(highlightColor);
            else
                sq.setFillColor((row + col) % 2 == 0 ? lightColor : darkColor);
            window.draw(sq);

            // ── Last-move highlight (olive-yellow glow) ─────────────────
            bool isLastMove = (lastMoveFrom.x != -1) &&
                              ((row == lastMoveFrom.x && col == lastMoveFrom.y) ||
                               (row == lastMoveTo.x   && col == lastMoveTo.y));
            if (isLastMove && selectedSquare.x != row && selectedSquare.y != col) {
                sf::RectangleShape lm(sf::Vector2f((float)TILE, (float)TILE));
                lm.setPosition(drawX, drawY);
                lm.setFillColor(sf::Color(highlightColor.r, highlightColor.g, highlightColor.b, 150));
                window.draw(lm);
            }

            // ── King-in-Check crimson warning box ─────────────────
            if (row == ckRow && col == ckCol) {
                sf::RectangleShape kc(sf::Vector2f((float)TILE, (float)TILE));
                kc.setPosition(drawX, drawY);
                kc.setFillColor(sf::Color(220, 30, 30, 165));
                window.draw(kc);
                sf::RectangleShape ko(sf::Vector2f((float)TILE - 6.f, (float)TILE - 6.f));
                ko.setPosition(drawX + 3.f, drawY + 3.f);
                ko.setFillColor(sf::Color::Transparent);
                ko.setOutlineThickness(2.5f);
                ko.setOutlineColor(sf::Color(255, 110, 70, 230));
                window.draw(ko);
            }

            // ── Move indicator dots / capture highlights ────────────────
            if (selectedSquare.x != -1 &&
                board.isValidMove(selectedSquare.x, selectedSquare.y, row, col)) {

                if (board.board[row][col] != '.') {
                    sf::RectangleShape cap(sf::Vector2f((float)TILE, (float)TILE));
                    cap.setPosition(drawX, drawY);
                    cap.setFillColor(sf::Color(210, 42, 48, 210));
                    window.draw(cap);
                } else {
                    sf::CircleShape dot(TILE / 7.0f);
                    dot.setFillColor(sf::Color(0, 0, 0, 55));
                    dot.setOrigin(dot.getRadius(), dot.getRadius());
                    dot.setPosition(drawX + TILE / 2.0f, drawY + TILE / 2.0f);
                    window.draw(dot);
                }
            }

            // ── Piece sprite ────────────────────────────────────────────
            char piece = board.board[row][col];

            // Skip the piece if it is currently animating or being dragged
            // (it will be drawn separately on top at the interpolated/cursor position)
            bool isFlyingPiece = false;
            if (isAnimating &&
                row == lastMoveTo.x && col == lastMoveTo.y)
                isFlyingPiece = true;   // slide animation destination after board state is committed
            if (isDragging &&
                row == dragSourceSquare.x && col == dragSourceSquare.y)
                isFlyingPiece = true;   // drag source

            if (piece != '.' && !isFlyingPiece) {
                sf::Sprite sprite(textures[piece]);
                float scale = (float)TILE / textures[piece].getSize().x * 0.8f;
                if (piece == 'k') scale *= 1.5f;
                if (piece == 'K') scale *= 2.0f;
                sprite.setScale(scale, scale);
                sprite.setOrigin(textures[piece].getSize().x / 2.0f,
                                 textures[piece].getSize().y / 2.0f);
                sprite.setPosition(drawX + TILE / 2.0f, drawY + TILE / 2.0f);
                window.draw(sprite);
            }
        }
    }

    // ── Floating piece: slide animation OR drag ──────────────────────────
    {
        char floatPiece = '.';
        sf::Vector2f floatPos;
        float floatScaleMult = 1.0f;

        if (isDragging) {
            // Drag: follow cursor exactly
            if (selectedPocketPiece != '\0') {
                floatPiece = selectedPocketPiece;
            } else {
                floatPiece = board.board[dragSourceSquare.x][dragSourceSquare.y];
            }
            floatPos       = dragCurrentPos;
            floatScaleMult = 1.08f;   // slightly lifted look
        } else if (isAnimating && animPiece != '.') {
            // Slide: linear interpolation
            float t  = animProgress;
            floatPos = animFrom + (animTo - animFrom) * t;
            floatPiece     = animPiece;
            floatScaleMult = 1.0f;
        }

        if (floatPiece != '.' && textures.count(floatPiece)) {
            sf::Sprite sp(textures[floatPiece]);
            float baseScale = (float)TILE / textures[floatPiece].getSize().x * 0.8f;
            if (floatPiece == 'k') baseScale *= 1.5f;
            if (floatPiece == 'K') baseScale *= 2.0f;
            float sc = baseScale * floatScaleMult;
            sp.setScale(sc, sc);
            sp.setOrigin(textures[floatPiece].getSize().x / 2.0f,
                         textures[floatPiece].getSize().y / 2.0f);
            sp.setPosition(floatPos);
            window.draw(sp);
        }
    }

    // Helper for piece values
    auto getVal = [](char p) {
        p = std::tolower(p);
        if (p == 'p') return 1;
        if (p == 'n' || p == 'b') return 3;
        if (p == 'r') return 5;
        if (p == 'q') return 9;
        return 0;
    };

    // Helper to draw capture row
    auto drawCaptures = [&](const std::vector<char>& captures, float x, float y, bool isWhiteRow) {
        std::vector<char> sorted = captures;
        std::sort(sorted.begin(), sorted.end(), [&](char a, char b) {
            return getVal(a) < getVal(b);
        });

        float curX = x;
        float capScale = 0.4f;
        for (char p : sorted) {
            sf::Sprite s(textures[p]);
            s.setScale(capScale, capScale);
            s.setPosition(curX, y);
            window.draw(s);
            curX += 25.0f;
        }

        int whiteMaterial = 0; for(char p : board.whiteCaptured) whiteMaterial += getVal(p);
        int blackMaterial = 0; for(char p : board.blackCaptured) blackMaterial += getVal(p);
        
        int diff = isWhiteRow ? (whiteMaterial - blackMaterial) : (blackMaterial - whiteMaterial);
        if (diff > 0) {
            whiteFont.drawText(window, "+" + std::to_string(diff), sf::Vector2f(curX + 10, y + 12), 0.25f);
        }
    };

    if (activeVariant == "Crazyhouse") {
        auto drawPocket = [&](bool isWhite) {
            const char* pieces = isWhite ? "PNBRQ" : "pnbrq";
            auto& pocket = isWhite ? board.whitePocket : board.blackPocket;
            
            for (int i = 0; i < 5; ++i) {
                char p = pieces[i];
                int count = pocket.count(p) ? pocket.at(p) : 0;
                
                sf::FloatRect rect = getPocketSlotRect(isWhite, i, offsetX, offsetY, boardSize);
                
                sf::RectangleShape box(sf::Vector2f(rect.width, rect.height));
                box.setPosition(rect.left, rect.top);
                
                bool isSel = (selectedPocketPiece == p);
                
                sf::Vector2i mi = sf::Mouse::getPosition(window);
                sf::Vector2f mp((float)mi.x, (float)mi.y);
                bool hovered = rect.contains(mp) && (board.isWhiteTurn == isWhite) && count > 0;
                
                if (isSel) {
                    box.setFillColor(sf::Color(173, 146, 29, 100));
                    box.setOutlineThickness(2.f);
                    box.setOutlineColor(sf::Color(210, 185, 60));
                } else if (hovered) {
                    box.setFillColor(sf::Color(45, 45, 55));
                    box.setOutlineThickness(1.5f);
                    box.setOutlineColor(sf::Color(180, 150, 90));
                } else {
                    box.setFillColor(sf::Color(25, 25, 30, 200));
                    box.setOutlineThickness(1.f);
                    box.setOutlineColor(sf::Color(70, 70, 80));
                }
                window.draw(box);
                
                if (textures.count(p)) {
                    sf::Sprite sp(textures[p]);
                    float spScale = (rect.width * 0.75f) / textures[p].getSize().x;
                    sp.setScale(spScale, spScale);
                    sp.setOrigin(textures[p].getSize().x / 2.f, textures[p].getSize().y / 2.f);
                    sp.setPosition(rect.left + rect.width / 2.f, rect.top + rect.height / 2.f);
                    
                    if (count == 0) {
                        sp.setColor(sf::Color(80, 80, 80, 100));
                    } else if (board.isWhiteTurn != isWhite) {
                        sp.setColor(sf::Color(180, 180, 180, 180));
                    }
                    
                    bool isDraggingThis = isDragging && selectedPocketPiece == p && board.isWhiteTurn == isWhite;
                    if (!isDraggingThis) {
                        window.draw(sp);
                    }
                }
                
                if (count > 0) {
                    std::string cntStr = std::to_string(count);
                    float txtScale = 0.12f;
                    float tw = whiteFont.getTextWidth(cntStr, txtScale);
                    whiteFont.drawText(window, cntStr, 
                                       sf::Vector2f(rect.left + rect.width - tw - 6.f, rect.top + 4.f), 
                                       txtScale, sf::Color(220, 220, 230));
                }
            }
        };
        
        drawPocket(false); // Black pocket
        drawPocket(true);  // White pocket
    } else {
        // Draw Black's trophies (White pieces captured) - TOP Row
        // Pushed higher to clear the new minimalist frame
        drawCaptures(board.blackCaptured, offsetX, offsetY - 80.0f, false);

        // Draw White's trophies (Black pieces captured) - BOTTOM Row
        // Nudged slightly higher for tighter spacing
        drawCaptures(board.whiteCaptured, offsetX, offsetY + boardSize + 40.0f, true);
    }

    // Turn Indicator - Pushed a bit further right for better spacing
    sf::Sprite turnSprite(board.isWhiteTurn ? whiteTurnTex : blackTurnTex);
    float rightSpace = (windowW - boardSize) / 2.0f;
    float turnScale = 1.0f;
    if (turnSprite.getTexture()->getSize().x > rightSpace * 0.8f) {
        turnScale = (rightSpace * 0.8f) / turnSprite.getTexture()->getSize().x;
    }
    turnSprite.setScale(turnScale, turnScale);
    turnSprite.setPosition(offsetX + boardSize + 60.0f, (windowH - turnSprite.getGlobalBounds().height) / 2.0f);
    window.draw(turnSprite);

    // Player Avatar Panels - aligned as one clean right-side stack
    float panelX = offsetX + boardSize + 65.0f;
    float avatarSize = 75.0f;
    float stackW = 220.f;
    float stackX = panelX;
    
    // Top right: Black player (Player 2). Name below avatar.
    float blackAvatarY = offsetY - 15.0f;
    drawPlayerPanel(stackX, blackAvatarY, player2Name, true);
    
    // Top right clock (Black): placed right beneath name
    float blackClockY = blackAvatarY + avatarSize + 12.0f + 26.0f;
    drawPlayerClock(stackX, blackClockY, blackTime, !board.isWhiteTurn);

    // Bottom right: White player (Player 1). Name above avatar.
    float whiteAvatarY = offsetY + boardSize - avatarSize + 15.0f;
    drawPlayerPanel(stackX, whiteAvatarY, player1Name, false);
    
    // Bottom right clock (White): placed right beneath avatar
    float whiteClockY = whiteAvatarY + avatarSize + 12.0f;
    drawPlayerClock(stackX, whiteClockY, whiteTime, board.isWhiteTurn);

    // In-game action buttons (left of board)
    renderActionButtons();

    // In-game gear button
    renderGearButton();

    // Move list and AI status
    renderMoveHistoryPanel();

    if (isVsAI) {
        bool stockfishIsTop = !aiPlaysWhite;
        float stockfishClockY = stockfishIsTop ? blackClockY : whiteClockY;
        float statusW = std::min(stackW, windowW - stackX - 24.f);
        float statusH = 64.f;
        float statusX = stackX;
        float statusY = stockfishIsTop ? (stockfishClockY + 48.f)
                                       : (whiteAvatarY - statusH - 46.f);
        float pulse = aiMovePending ? (0.55f + 0.45f * std::sin(uiPulseTime * 7.f)) : 0.f;
        sf::Color outline = aiMovePending
            ? sf::Color(210, 185, 60, (sf::Uint8)(150 + 70 * pulse))
            : sf::Color(70, 70, 82);
        ui::drawRoundedPanel(window, {statusX, statusY, statusW, statusH}, 5.f,
                             sf::Color(20, 20, 26, 225), outline, 1.1f);

        std::string level = "AI LEVEL " + std::to_string(aiSkillLevel) + " / 20";
        whiteFont.drawText(window, level, {statusX + 12.f, statusY + 10.f}, 0.12f, sf::Color(190, 190, 205));

        std::string state = aiMovePending ? "STOCKFISH THINKING..." : "STOCKFISH READY";
        whiteFont.drawText(window, state, {statusX + 12.f, statusY + 36.f}, 0.12f,
                           aiMovePending ? sf::Color(220, 195, 120) : sf::Color(140, 170, 130));
    }
}

void Game::drawPlayerPanel(float x, float y, const std::string& name, bool nameBelow) {
    float avatarSize = 75.0f;
    float padding = 12.0f;

    // Avatar Drop Shadow (Soft, minimalist depth)
    ui::drawRoundedRect(window, {x + 5.0f, y + 5.0f, avatarSize, avatarSize}, 6.f,
                        sf::Color(0, 0, 0, 60));

    // Avatar Placeholder Box
    ui::drawRoundedPanel(window, {x, y, avatarSize, avatarSize}, 6.f,
                         sf::Color(35, 35, 40),
                         sf::Color(80, 80, 90),
                         1.4f);

    // Player Name
    float nameScale = 0.18f;
    float nw = whiteFont.getTextWidth(name, nameScale);
    
    // Center name horizontally with respect to the avatar box
    float textX = x + (avatarSize / 2.0f) - (nw / 2.0f);
    // Position vertically based on nameBelow flag
    float textY = nameBelow ? (y + avatarSize + padding) : (y - padding - 15.0f);
    
    // Render clean name text
    whiteFont.drawText(window, name, sf::Vector2f(textX, textY), nameScale, sf::Color(220, 220, 230));
}

void Game::renderGearButton() {
    // A sleek minimal circular gear/settings button in the top-left corner
    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mp(mi);

    float bx = 38.f, by = 32.f, bSz = 42.f;
    bool hovered = sf::FloatRect(bx, by, bSz, bSz).contains(mp);

    float ease = sessionMenuAnim * sessionMenuAnim * (3.f - 2.f * sessionMenuAnim);
    const char* labels[] = { "SETTINGS", "ABORT", "QUIT" };
    sf::Color fills[] = {
        sf::Color(28, 52, 88, (sf::Uint8)(230 * ease)),
        sf::Color(84, 58, 22, (sf::Uint8)(230 * ease)),
        sf::Color(86, 32, 34, (sf::Uint8)(230 * ease))
    };
    sf::Color outlines[] = {
        sf::Color(70, 130, 190, (sf::Uint8)(220 * ease)),
        sf::Color(185, 130, 45, (sf::Uint8)(220 * ease)),
        sf::Color(190, 80, 82, (sf::Uint8)(220 * ease))
    };
    float optW = 98.f, optH = 34.f, optGap = 8.f;
    float optBaseX = bx + bSz + 14.f;
    float optY = by + (bSz - optH) / 2.f;

    for (int i = 2; i >= 0; --i) {
        float closedX = bx + bSz * 0.35f;
        float openX = optBaseX + i * (optW + optGap);
        float x = closedX + (openX - closedX) * ease;
        sf::FloatRect rect(x, optY, optW, optH);
        bool optionHover = sessionMenuAnim > 0.85f && rect.contains(mp);

        ui::drawRoundedRect(window, {x + 3.f * ease, optY + 4.f * ease, optW, optH}, 5.f,
                            sf::Color(0, 0, 0, (sf::Uint8)(65 * ease)));
        ui::drawRoundedPanel(window, {x, optY, optW, optH}, 5.f,
                             optionHover ? sf::Color(173, 146, 29, (sf::Uint8)(245 * ease)) : fills[i],
                             optionHover ? sf::Color(230, 205, 95, (sf::Uint8)(245 * ease)) : outlines[i],
                             1.2f);

        float sc = 0.12f;
        float tw = whiteFont.getTextWidth(labels[i], sc);
        sf::Color tc = optionHover ? sf::Color(14, 14, 18, (sf::Uint8)(255 * ease))
                                   : sf::Color(220, 220, 232, (sf::Uint8)(230 * ease));
        whiteFont.drawText(window, labels[i], {x + optW / 2.f - tw / 2.f, optY + 9.f}, sc, tc);
    }

    // Outer circle (background disc)
    sf::CircleShape disc(bSz / 2.f);
    disc.setPosition(bx, by);
    disc.setFillColor((hovered || sessionMenuOpen) ? sf::Color(173, 146, 29, 220) : sf::Color(25, 25, 32, 220));
    disc.setOutlineThickness(1.5f);
    disc.setOutlineColor((hovered || sessionMenuOpen) ? sf::Color(230, 205, 90) : sf::Color(80, 80, 90));
    window.draw(disc);

    // Minimal menu mark. The bars rotate as the session actions slide out.
    sf::Vector2f center(bx + bSz / 2.f, by + bSz / 2.f);
    sf::Color barColor = (hovered || sessionMenuOpen) ? sf::Color(14, 14, 18) : sf::Color(210, 210, 220);
    for (int i = 0; i < 3; ++i) {
        sf::RectangleShape bar({18.f, 2.5f});
        bar.setOrigin(9.f, 1.25f);
        float yOff = (i - 1) * (6.f - 3.f * ease);
        bar.setPosition(center.x, center.y + yOff);
        bar.setRotation(90.f * ease);
        bar.setFillColor(barColor);
        window.draw(bar);
    }

}

// ─── Transition System ────────────────────────────────────────────────────
void Game::startTransition(State nextState) {
    transOut      = true;
    transNextState = nextState;
    transAlpha     = 0.f;   // Start from transparent → fade to black first
}

void Game::updateTransition(float dt) {
    if (!transOut && transAlpha <= 0.f) return; // Nothing to animate

    const float speed = 520.f; // Pixels/sec alpha change — snappy but smooth

    if (transOut) {
        transAlpha += dt * speed;
        if (transAlpha >= 255.f) {
            // Mid-point: switch state
            transAlpha = 255.f;
            currentState = transNextState;
            transOut = false;  // Now fade back in
        }
    } else {
        transAlpha -= dt * speed;
        if (transAlpha < 0.f) transAlpha = 0.f;
    }
}

void Game::drawTransitionOverlay() {
    if (transAlpha <= 0.f) return;
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    sf::Uint8 a = (sf::Uint8)(transAlpha > 255.f ? 255 : (int)transAlpha);
    sf::RectangleShape veil({W, H});
    veil.setFillColor(sf::Color(10, 10, 14, a));
    window.draw(veil);
}

// ─── In-Game Action Buttons ───────────────────────────────────────────────
void Game::renderActionButtons() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    float boardSize = 8.0f * TILE;
    float offsetX = (W - boardSize) / 2.0f;
    float offsetY = (H - boardSize) / 2.0f;
    float panelW = std::min(260.f, std::max(160.f, offsetX - 72.f));
    float panelX = std::max(16.f, offsetX - panelW - 50.f);
    float panelY = offsetY + 172.f;
    if (panelW < 150.f) return;

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mp(mi);

    bool canUndo = (currentMoveIndex > 0);
    bool canRedo = (currentMoveIndex < (int)stateHistory.size() - 1);

    struct ActionBtn { std::string label; bool disabled; };
    ActionBtn btns[] = {
        {"UNDO",  !canUndo},
        {"REDO",  !canRedo}
    };
    int nBtns = 2;

    float btnW = (panelW - 10.f) / 2.f;
    float btnH = 34.f;
    float gap  = 10.f;
    float bx = panelX;
    float by = panelY - btnH - 10.f;

    for (int i = 0; i < nBtns; ++i) {
        float btnX = bx + i * (btnW + gap);
        bool  hov = sf::FloatRect(btnX, by, btnW, btnH).contains(mp) && !btns[i].disabled;
        bool  dis = btns[i].disabled;

        ui::drawRoundedPanel(window, {btnX, by, btnW, btnH}, 5.f,
                             dis ? sf::Color(20, 20, 25) :
                             (hov ? sf::Color(173, 146, 29) : sf::Color(28, 28, 36)),
                             dis ? sf::Color(42, 42, 50) :
                             (hov ? sf::Color(210, 185, 60) : sf::Color(70, 70, 82)),
                             1.f);

        // Label
        float sc = 0.15f;
        float tw = whiteFont.getTextWidth(btns[i].label, sc);
        sf::Color col = dis ? sf::Color(55, 55, 65) :
                        (hov ? sf::Color(14, 14, 18) : sf::Color(200, 200, 215));
        whiteFont.drawText(window, btns[i].label,
                           {btnX + btnW / 2.f - tw / 2.f, by + btnH / 2.f - 7.f},
                           sc, col);
    }
}

int Game::getFirstVisibleHistoryPosition(int maxRows) const {
    int totalPositions = (int)moveList.size() + 1;
    if (totalPositions <= maxRows) return 0;

    int first = currentMoveIndex - maxRows / 2;
    if (first < 0) first = 0;
    if (first + maxRows > totalPositions) first = totalPositions - maxRows;
    return first;
}

void Game::renderMoveHistoryPanel() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    float boardSize = 8.0f * TILE;
    float offsetX = (W - boardSize) / 2.0f;
    float offsetY = (H - boardSize) / 2.0f;

    float panelW = std::min(260.f, std::max(160.f, offsetX - 72.f));
    float panelX = std::max(16.f, offsetX - panelW - 50.f);
    if (panelW < 150.f) return;

    float panelY = offsetY + 172.f;
    float panelH = std::min(420.f, offsetY + boardSize - panelY - 18.f);
    if (panelH < 120.f) return;
    float headerH = 48.f;
    float rowH = 26.f;

    ui::drawRoundedRect(window, {panelX + 5.f, panelY + 6.f, panelW, panelH}, 6.f,
                        sf::Color(0, 0, 0, 70));
    ui::drawRoundedPanel(window, {panelX, panelY, panelW, panelH}, 6.f,
                         sf::Color(20, 20, 26, 238),
                         sf::Color(80, 80, 92),
                         1.2f);

    whiteFont.drawText(window, "MOVE HISTORY", {panelX + 14.f, panelY + 12.f}, 0.14f, sf::Color(220, 195, 120));

    std::string reviewText = "PLY " + std::to_string(currentMoveIndex) + " / " + std::to_string((int)moveList.size());
    whiteFont.drawText(window, reviewText, {panelX + panelW - 96.f, panelY + 13.f}, 0.10f,
                       currentMoveIndex == (int)moveList.size() ? sf::Color(120, 130, 145) : sf::Color(220, 195, 120));

    sf::RectangleShape divider({panelW - 24.f, 1.f});
    divider.setPosition(panelX + 12.f, panelY + headerH - 6.f);
    divider.setFillColor(sf::Color(70, 70, 82));
    window.draw(divider);

    float listStartY = panelY + headerH;
    if (currentMoveIndex < (int)moveList.size()) {
        float btnW = panelW - 24.f;
        float btnH = 30.f;
        float btnY = listStartY + 2.f;
        sf::Vector2f mpNow((float)sf::Mouse::getPosition(window).x, (float)sf::Mouse::getPosition(window).y);
        bool btnHover = sf::FloatRect(panelX + 12.f, btnY, btnW, btnH).contains(mpNow);
        ui::drawRoundedPanel(window, {panelX + 12.f, btnY, btnW, btnH}, 5.f,
                             btnHover ? sf::Color(173, 146, 29) : sf::Color(32, 55, 92),
                             btnHover ? sf::Color(220, 195, 90) : sf::Color(58, 105, 170),
                             1.f);

        std::string label = "RETURN TO LIVE";
        float ls = 0.12f;
        float lw = whiteFont.getTextWidth(label, ls);
        whiteFont.drawText(window, label, {panelX + 12.f + btnW / 2.f - lw / 2.f, btnY + 8.f},
                           ls, btnHover ? sf::Color(14, 14, 18) : sf::Color(200, 220, 245));
        listStartY += btnH + 8.f;
    }

    int maxRows = std::max(1, (int)((panelY + panelH - listStartY - 12.f) / rowH));
    int firstPosition = getFirstVisibleHistoryPosition(maxRows);
    int totalPositions = (int)moveList.size() + 1;
    int endPosition = std::min(totalPositions, firstPosition + maxRows);
    sf::Vector2f mp((float)sf::Mouse::getPosition(window).x, (float)sf::Mouse::getPosition(window).y);

    for (int pos = firstPosition; pos < endPosition; ++pos) {
        int visibleRow = pos - firstPosition;
        float rowY = listStartY + visibleRow * rowH;
        sf::FloatRect rowRect(panelX + 8.f, rowY, panelW - 16.f, rowH - 2.f);
        bool active = pos == currentMoveIndex;
        bool hover = rowRect.contains(mp);

        if (active || hover) {
            ui::drawRoundedRect(window, rowRect, 4.f,
                                active ? sf::Color(173, 146, 29, 145) : sf::Color(45, 45, 55, 210));
        }

        std::string text;
        if (pos == 0) {
            text = "START POSITION";
        } else {
            int moveIdx = pos - 1;
            if (moveIdx % 2 == 0) text = std::to_string(moveIdx / 2 + 1) + ". " + moveList[moveIdx];
            else text = std::to_string(moveIdx / 2 + 1) + "... " + moveList[moveIdx];
        }

        sf::Color textColor = active ? sf::Color(12, 12, 16) : sf::Color(205, 205, 218);
        whiteFont.drawText(window, text, {panelX + 16.f, rowY + 6.f}, 0.115f, textColor);
    }
}

bool Game::handleMoveHistoryClick(const sf::Vector2f& mp) {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    float boardSize = 8.0f * TILE;
    float offsetX = (W - boardSize) / 2.0f;
    float offsetY = (H - boardSize) / 2.0f;

    float panelW = std::min(260.f, std::max(160.f, offsetX - 72.f));
    float panelX = std::max(16.f, offsetX - panelW - 50.f);
    if (panelW < 150.f) return false;

    float panelY = offsetY + 172.f;
    float panelH = std::min(420.f, offsetY + boardSize - panelY - 18.f);
    if (panelH < 120.f) return false;
    float headerH = 48.f;
    float rowH = 26.f;

    float actionBtnH = 34.f;
    float actionBtnW = (panelW - 10.f) / 2.f;
    float actionY = panelY - actionBtnH - 10.f;
    if (sf::FloatRect(panelX, actionY, actionBtnW, actionBtnH).contains(mp)) {
        if (currentMoveIndex > 0) {
            int step = (isVsAI && currentMoveIndex >= 2) ? 2 : 1;
            setReviewPosition(std::max(0, currentMoveIndex - step));
        }
        return true;
    }
    if (sf::FloatRect(panelX + actionBtnW + 10.f, actionY, actionBtnW, actionBtnH).contains(mp)) {
        if (currentMoveIndex < (int)stateHistory.size() - 1) {
            int maxIndex = (int)stateHistory.size() - 1;
            int step = (isVsAI && currentMoveIndex + 2 <= maxIndex) ? 2 : 1;
            setReviewPosition(std::min(maxIndex, currentMoveIndex + step));
        }
        return true;
    }

    if (!sf::FloatRect(panelX, panelY, panelW, panelH).contains(mp)) return false;
    if (mp.y < panelY + headerH) return true;

    float listStartY = panelY + headerH;
    if (currentMoveIndex < (int)moveList.size()) {
        float btnW = panelW - 24.f;
        float btnH = 30.f;
        float btnY = listStartY + 2.f;
        if (sf::FloatRect(panelX + 12.f, btnY, btnW, btnH).contains(mp)) {
            setReviewPosition((int)moveList.size());
            return true;
        }
        listStartY += btnH + 8.f;
    }

    int maxRows = std::max(1, (int)((panelY + panelH - listStartY - 12.f) / rowH));
    int firstPosition = getFirstVisibleHistoryPosition(maxRows);
    int row = (int)((mp.y - listStartY) / rowH);
    int position = firstPosition + row;
    int totalPositions = (int)moveList.size() + 1;
    if (position >= 0 && position < totalPositions) setReviewPosition(position);
    return true;
}

void Game::setReviewPosition(int positionIndex) {
    if (positionIndex < 0 || positionIndex >= (int)stateHistory.size()) return;
    aiMovePending = false;
    isAnimating = false;
    isDragging = false;
    selectedSquare = { -1, -1 };
    selectedPocketPiece = '\0';
    currentMoveIndex = positionIndex;
    board = stateHistory[currentMoveIndex];
    updateLastMoveHighlight();
    saveCurrentGamePGN();
}

void Game::ensureDirectoriesExist() {
    std::filesystem::create_directories("data/saves");
    std::filesystem::create_directories("data/config");
}

void Game::loadConfig() {
    std::ifstream file("data/config/config.txt");
    if (!file.is_open()) {
        saveConfig();
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };
        trim(key);
        trim(val);
        
        if (key == "time_control") {
            try {
                initialTimeLimit = std::stof(val);
                whiteTime = initialTimeLimit;
                blackTime = initialTimeLimit;
            } catch (...) {}
        } else if (key == "increment_time") {
            try { incrementTime = std::stof(val); } catch (...) {}
        } else if (key == "delay_time") {
            try {
                delayTime = std::stof(val);
                activeDelayRemaining = delayTime;
            } catch (...) {}
        } else if (key == "white_wins") {
            try { statWhiteWins = std::stoi(val); } catch (...) {}
        } else if (key == "black_wins") {
            try { statBlackWins = std::stoi(val); } catch (...) {}
        } else if (key == "draws") {
            try { statDraws = std::stoi(val); } catch (...) {}
        } else if (key == "total_games") {
            try { statTotalGames = std::stoi(val); } catch (...) {}
        } else if (key == "menu_background" || key == "board_tile_theme" || key == "board_background" ||
                   key == "board_perspective") {
            if (settingsScreen) {
                int rowIdx = -1;
                if (key == "menu_background") rowIdx = 0;
                else if (key == "board_tile_theme") rowIdx = 1;
                else if (key == "board_background") rowIdx = 2;
                else if (key == "board_perspective") rowIdx = 3;
                if (rowIdx != -1) {
                    settingsScreen->setSelection(rowIdx, val);
                }
            }
        }
    }
}

void Game::saveConfig() {
    std::ofstream file("data/config/config.txt");
    if (!file.is_open()) return;
    
    file << "# User Preferences\n";
    if (settingsScreen) {
        file << "menu_background=" << settingsScreen->getMenuBg() << "\n";
        file << "board_tile_theme=" << settingsScreen->getBoardTileTheme() << "\n";
        file << "board_background=" << settingsScreen->getBoardBg() << "\n";
        file << "board_perspective=" << settingsScreen->getBoardPerspective() << "\n";
    } else {
        file << "menu_background=Default\n";
        file << "board_tile_theme=Classic\n";
        file << "board_background=Texture\n";
        file << "board_perspective=White\n";
    }
    
    file << "\n# Player Clocks (in seconds)\n";
    file << "time_control=" << (int)initialTimeLimit << "\n";
    file << "increment_time=" << (int)incrementTime << "\n";
    file << "delay_time=" << (int)delayTime << "\n";
    
    file << "\n# Player Statistics\n";
    file << "white_wins=" << statWhiteWins << "\n";
    file << "black_wins=" << statBlackWins << "\n";
    file << "draws=" << statDraws << "\n";
    file << "total_games=" << statTotalGames << "\n";
}

void Game::saveCurrentGamePGN() {
    if (currentPGNPath.empty()) return;
    std::ofstream file(currentPGNPath);
    if (!file.is_open()) return;
    file << generatePGNContent((int)moveList.size());
}

static std::string getBoardFEN(const ChessBoard& b) {
    std::string fen = "";
    for (int r = 0; r < 8; ++r) {
        int emptyCount = 0;
        for (int c = 0; c < 8; ++c) {
            char p = b.board[r][c];
            if (p == '.') {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                fen += p;
            }
        }
        if (emptyCount > 0) {
            fen += std::to_string(emptyCount);
        }
        if (r < 7) fen += "/";
    }
    return fen;
}

std::string Game::generatePGNContent(int limit) {
    std::stringstream ss;
    int safeLimit = std::max(0, std::min(limit, (int)moveList.size()));
    int reviewPly = std::max(0, std::min(currentMoveIndex, safeLimit));
    const ChessBoard& resultBoard =
        (!stateHistory.empty() && safeLimit < (int)stateHistory.size()) ? stateHistory[safeLimit] : board;

    ss << "[Event \"Casual Match\"]\n";
    ss << "[Site \"Local Machine\"]\n";
    
    time_t t = time(0);
    struct tm * now = localtime(&t);
    char dateBuf[80];
    if (now) {
        strftime(dateBuf, sizeof(dateBuf), "%Y.%m.%d", now);
        ss << "[Date \"" << dateBuf << "\"]\n";
    } else {
        ss << "[Date \"2026.05.25\"]\n";
    }
    
    ss << "[Round \"1\"]\n";
    ss << "[White \"" << player1Name << "\"]\n";
    ss << "[Black \"" << player2Name << "\"]\n";
    
    int whiteAv = setupScreen ? 0 : 0;
    int blackAv = setupScreen ? 0 : 0;
    ss << "[WhiteAvatar \"" << whiteAv << "\"]\n";
    ss << "[BlackAvatar \"" << blackAv << "\"]\n";
    
    if (activeVariant != "Standard") {
        ss << "[Variant \"" << activeVariant << "\"]\n";
    }
    if (activeVariant == "Chess 960" && !stateHistory.empty()) {
        ss << "[SetUp \"1\"]\n";
        ss << "[FEN \"" << getBoardFEN(stateHistory[0]) << "\"]\n";
        ss << "[InitialKingCol \"" << stateHistory[0].initialKingCol << "\"]\n";
        ss << "[InitialRookLCol \"" << stateHistory[0].initialRookLCol << "\"]\n";
        ss << "[InitialRookRCol \"" << stateHistory[0].initialRookRCol << "\"]\n";
    }
    
    bool isWhiteKingInCenter = false;
    bool isBlackKingInCenter = false;
    for (int r = 3; r <= 4; ++r) {
        for (int c = 3; c <= 4; ++c) {
            if (resultBoard.board[r][c] == 'K') isWhiteKingInCenter = true;
            if (resultBoard.board[r][c] == 'k') isBlackKingInCenter = true;
        }
    }
    bool kingOfHillWhiteWin = (activeVariant == "King of the Hill" && isWhiteKingInCenter);
    bool kingOfHillBlackWin = (activeVariant == "King of the Hill" && isBlackKingInCenter);

    bool threeCheckWhiteWin = (activeVariant == "3-Check" && resultBoard.checksDeliveredByWhite >= 3);
    bool threeCheckBlackWin = (activeVariant == "3-Check" && resultBoard.checksDeliveredByBlack >= 3);
    
    std::string result = "*";
    if (whiteTime <= 0.f && !isUnlimitedTime) result = "0-1";
    else if (blackTime <= 0.f && !isUnlimitedTime) result = "1-0";
    else if (resultBoard.isCheckmate(true)) result = "0-1";
    else if (resultBoard.isCheckmate(false)) result = "1-0";
    else if (kingOfHillWhiteWin) result = "1-0";
    else if (kingOfHillBlackWin) result = "0-1";
    else if (threeCheckWhiteWin) result = "1-0";
    else if (threeCheckBlackWin) result = "0-1";
    else if (resultBoard.isStalemate(true) || resultBoard.isStalemate(false) ||
             resultBoard.halfmoveClock >= 100 || checkThreefoldRepetition() ||
             resultBoard.isInsufficientMaterial()) {
        result = "1/2-1/2";
    }
    ss << "[Result \"" << result << "\"]\n";
    ss << "[WhiteTime \"" << (int)whiteTime << "\"]\n";
    ss << "[BlackTime \"" << (int)blackTime << "\"]\n";
    ss << "[TimeControl \"" << (int)initialTimeLimit << "\"]\n";
    ss << "[IncrementTime \"" << (int)incrementTime << "\"]\n";
    ss << "[DelayTime \"" << (int)delayTime << "\"]\n";
    ss << "[ReviewPly \"" << reviewPly << "\"]\n\n";

    for (int i = 0; i < safeLimit; ++i) {
        if (i % 2 == 0) {
            ss << (i / 2 + 1) << ". " << moveList[i];
        } else {
            ss << " " << moveList[i] << "\n";
        }
    }
    if (safeLimit % 2 != 0) {
        ss << "\n";
    }
    
    return ss.str();
}

void Game::drawPlayerClock(float x, float y, float timeRemaining, bool isActive) {
    float width = 75.f;
    float height = 34.f;
    
    sf::Color outlineCol;
    sf::Color textCol;
    
    if (isUnlimitedTime) {
        outlineCol = sf::Color(65, 65, 75); // muted gray
        textCol = sf::Color(140, 140, 150);
    } else if (timeRemaining < 30.f) {
        // Warning: low time! Pulse outline using sine wave
        static float pulseTimer = 0.f;
        pulseTimer += 0.016f; // approx 60fps delta
        int alpha = 130 + (int)(125.f * std::sin(pulseTimer * 10.f));
        outlineCol = sf::Color(220, 50, 50, alpha);
        textCol = sf::Color(240, 70, 70);
    } else if (isActive) {
        outlineCol = sf::Color(173, 146, 29); // gold
        textCol = sf::Color(220, 195, 120);
    } else {
        outlineCol = sf::Color(65, 65, 75); // muted gray
        textCol = sf::Color(140, 140, 150);
    }
    
    ui::drawRoundedPanel(window, {x, y, width, height}, 5.f,
                         sf::Color(22, 22, 26, 220),
                         outlineCol,
                         1.2f);
    
    // Format clock text
    std::string timeStr;
    if (isUnlimitedTime) {
        timeStr = "--:--";
    } else {
        int mins = (int)(timeRemaining / 60.f);
        int secs = (int)timeRemaining % 60;
        
        if (timeRemaining < 10.f) {
            // High fidelity format showing tenths of a second
            int tenths = (int)(timeRemaining * 10.f) % 10;
            timeStr = std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs) + "." + std::to_string(tenths);
        } else {
            timeStr = std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
        }
    }
    
    float scale = 0.145f;
    float tw = whiteFont.getTextWidth(timeStr, scale);
    if (tw > width - 10.f && tw > 0.f) {
        scale *= (width - 10.f) / tw;
        tw = whiteFont.getTextWidth(timeStr, scale);
    }
    float textX = x + (width / 2.f) - (tw / 2.f);
    float textY = y + (height - scale * 50.f) / 2.f;
    
    whiteFont.drawText(window, timeStr, sf::Vector2f(textX, textY), scale, textCol);
}

std::string Game::makeUCIMove(int fromRow, int fromCol, int toRow, int toCol, char promoChar) const {
    if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 ||
        toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8) {
        return "";
    }

    std::string move;
    move += (char)('a' + fromCol);
    move += (char)('8' - fromRow);
    move += (char)('a' + toCol);
    move += (char)('8' - toRow);
    if (promoChar != '\0') {
        move += (char)std::tolower((unsigned char)promoChar);
    }
    return move;
}

bool Game::isHumanTurn() const {
    if (currentMoveIndex < (int)moveList.size()) return false;
    if (!isVsAI) return true;
    if (aiMovePending) return false;
    return board.isWhiteTurn != aiPlaysWhite;
}

void Game::appendMoveToTimeline(const std::string& lan, const std::string& uci) {
    if (currentMoveIndex < (int)stateHistory.size() - 1) {
        stateHistory.erase(stateHistory.begin() + currentMoveIndex + 1, stateHistory.end());
        moveList.erase(moveList.begin() + currentMoveIndex, moveList.end());
    }
    if (currentMoveIndex < (int)uciMoveHistory.size()) {
        uciMoveHistory.erase(uciMoveHistory.begin() + currentMoveIndex, uciMoveHistory.end());
    }

    moveList.push_back(lan);
    if (!uci.empty()) {
        uciMoveHistory.push_back(uci);
    }
    stateHistory.push_back(board);
    currentMoveIndex++;
    saveCurrentGamePGN();
}

void Game::requestAIMoveIfNeeded() {
    if (!isVsAI || activeVariant != "Standard" || currentState != State::PLAYING) return;
    if (currentMoveIndex < (int)moveList.size()) return;
    if (board.isWhiteTurn != aiPlaysWhite) return;
    if (aiMovePending) return;

    bool gameOver = board.isCheckmate(board.isWhiteTurn) ||
                    board.isStalemate(board.isWhiteTurn) ||
                    board.halfmoveClock >= 100 ||
                    checkThreefoldRepetition() ||
                    board.isInsufficientMaterial() ||
                    (whiteTime <= 0.f && !isUnlimitedTime) ||
                    (blackTime <= 0.f && !isUnlimitedTime);
    if (gameOver) return;

    aiMovePending = true;
    aiMoveDelayRemaining = aiMoveDelaySeconds;
}

void Game::executePendingAIMove() {
    if (!isVsAI || activeVariant != "Standard" || currentState != State::PLAYING) return;
    if (board.isWhiteTurn != aiPlaysWhite) return;

    std::string bestMove;
    if (aiSkillLevel <= 5) {
        bool useWeakMove = aiSkillLevel <= 2;
        if (aiSkillLevel > 2) {
            static std::mt19937 rng(std::random_device{}());
            int randomChance = 90 - aiSkillLevel * 12;
            useWeakMove = std::uniform_int_distribution<int>(0, 99)(rng) < randomChance;
        }
        if (useWeakMove && chooseWeakAIMove(bestMove)) {
            if (!applyAIMove(bestMove)) {
                std::cerr << "Weak AI generated an invalid move: " << bestMove << "\n";
            }
            return;
        }
    }

    if (!stockfish.isRunning() &&
        !stockfish.start("stockfish/stockfish-windows-x86-64-avx2.exe")) {
        std::cerr << "Stockfish unavailable: " << stockfish.getLastError() << "\n";
        if (chooseWeakAIMove(bestMove)) applyAIMove(bestMove);
        return;
    }
    stockfish.setSkillLevel(aiSkillLevel);

    int historyLimit = std::min(currentMoveIndex, (int)uciMoveHistory.size());
    std::vector<std::string> activeUciHistory(uciMoveHistory.begin(), uciMoveHistory.begin() + historyLimit);
    bestMove = stockfish.getBestMove(activeUciHistory, std::max(1, aiSearchDepth));
    if (bestMove.empty()) {
        std::cerr << "Stockfish failed to provide a move: " << stockfish.getLastError() << "\n";
        if (chooseWeakAIMove(bestMove)) applyAIMove(bestMove);
        return;
    }

    if (!applyAIMove(bestMove)) {
        std::cerr << "Stockfish returned an invalid move: " << bestMove << "\n";
    }
}

bool Game::chooseWeakAIMove(std::string& uciMove) const {
    std::vector<std::string> legalMoves;
    for (int fromR = 0; fromR < 8; ++fromR) {
        for (int fromC = 0; fromC < 8; ++fromC) {
            char piece = board.board[fromR][fromC];
            if (piece == '.') continue;
            bool isWhitePiece = std::isupper((unsigned char)piece);
            if (isWhitePiece != board.isWhiteTurn) continue;

            for (int toR = 0; toR < 8; ++toR) {
                for (int toC = 0; toC < 8; ++toC) {
                    if (!board.isValidMove(fromR, fromC, toR, toC)) continue;
                    char promo = '\0';
                    if ((piece == 'P' && toR == 0) || (piece == 'p' && toR == 7)) {
                        promo = 'q';
                    }
                    legalMoves.push_back(makeUCIMove(fromR, fromC, toR, toC, promo));
                }
            }
        }
    }

    if (legalMoves.empty()) return false;
    static std::mt19937 rng(std::random_device{}());
    uciMove = legalMoves[std::uniform_int_distribution<int>(0, (int)legalMoves.size() - 1)(rng)];
    return true;
}

bool Game::applyAIMove(const std::string& uciMove) {
    if (uciMove.size() < 4) return false;

    int fromCol = uciMove[0] - 'a';
    int fromRow = 8 - (uciMove[1] - '0');
    int toCol = uciMove[2] - 'a';
    int toRow = 8 - (uciMove[3] - '0');
    char promoChar = (uciMove.size() >= 5) ? uciMove[4] : '\0';

    if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 ||
        toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8) {
        return false;
    }
    if (!board.isValidMove(fromRow, fromCol, toRow, toCol)) {
        return false;
    }

    char movingPiece = board.board[fromRow][fromCol];
    std::string lan = board.getLANMove(fromRow, fromCol, toRow, toCol, promoChar);

    bool needsPromotion = board.movePiece(fromRow, fromCol, toRow, toCol);
    if (needsPromotion) {
        char promoteTo = promoChar == '\0' ? 'q' : promoChar;
        promoteTo = board.isWhiteTurn ? (char)std::toupper((unsigned char)promoteTo)
                                      : (char)std::tolower((unsigned char)promoteTo);
        board.promotePiece(toRow, toCol, promoteTo);
    }

    lastMoveFrom = { fromRow, fromCol };
    lastMoveTo = { toRow, toCol };
    animPiece = movingPiece;
    animFrom = getSquareScreenPos(fromRow, fromCol) + sf::Vector2f(TILE / 2.f, TILE / 2.f);
    animTo = getSquareScreenPos(toRow, toCol) + sf::Vector2f(TILE / 2.f, TILE / 2.f);
    animProgress = 0.f;
    isAnimating = true;

    isTimerRunning = true;
    if (!board.isWhiteTurn) {
        whiteTime += incrementTime;
    } else {
        blackTime += incrementTime;
    }
    activeDelayRemaining = delayTime;

    appendMoveToTimeline(lan, uciMove);
    return true;
}

void Game::scanSavedMatches() {
    savedMatches.clear();
    if (!std::filesystem::exists("data/saves")) return;
    
    std::vector<std::string> paths;
    for (const auto& entry : std::filesystem::directory_iterator("data/saves")) {
        if (entry.path().extension() == ".pgn") {
            paths.push_back(entry.path().string());
        }
    }
    
    // Reverse alphabetical sort so newest files are first
    std::sort(paths.begin(), paths.end(), std::greater<std::string>());
    
    for (const auto& p : paths) {
        SavedMatchInfo info;
        info.filePath = p;
        
        std::ifstream file(p);
        if (file.is_open()) {
            std::string line;
            std::string movesContent = "";
            while (std::getline(file, line)) {
                if (line.empty()) continue;
                if (line[0] == '[') {
                    size_t space = line.find(' ');
                    if (space != std::string::npos) {
                        std::string tag = line.substr(1, space - 1);
                        size_t firstQuote = line.find('"');
                        size_t lastQuote = line.rfind('"');
                        if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
                            std::string val = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                            if (tag == "White") info.player1 = val;
                            else if (tag == "Black") info.player2 = val;
                            else if (tag == "Date") info.date = val;
                            else if (tag == "Result") info.result = val;
                        }
                    }
                } else {
                    movesContent += line + " ";
                }
            }
            
            // Count moves
            std::stringstream ss(movesContent);
            std::string token;
            int moveCount = 0;
            while (ss >> token) {
                if (token.find('.') == std::string::npos && 
                    token != "*" && token != "1-0" && token != "0-1" && token != "1/2-1/2" &&
                    !token.empty()) {
                    moveCount++;
                }
            }
            info.moveCount = moveCount;
        }
        savedMatches.push_back(info);
    }
}

static void setupBoardFromFEN(ChessBoard& b, const std::string& fen) {
    for (int r = 0; r < 8; ++r) {
        b.board[r] = "........";
    }
    int r = 0, c = 0;
    for (char ch : fen) {
        if (ch == ' ') break; // only parse piece layout part
        if (ch == '/') {
            r++;
            c = 0;
            if (r >= 8) break;
        } else if (std::isdigit(ch)) {
            c += (ch - '0');
        } else {
            if (r < 8 && c < 8) {
                b.board[r][c] = ch;
                c++;
            }
        }
    }
}

void Game::loadGameFromPGN(const std::string& path) {
    isVsAI = false;
    aiMovePending = false;
    activeVariant = "Standard"; // default
    board = ChessBoard();
    stateHistory.clear();
    moveList.clear();
    uciMoveHistory.clear();
    stateHistory.push_back(board);
    currentMoveIndex = 0;

    float loadedWhiteTime = -1.f;
    float loadedBlackTime = -1.f;
    float loadedTimeControl = -1.f;
    int loadedReviewPly = -1;

    std::ifstream file(path);
    if (!file.is_open()) return;
    
    std::string line;
    std::string movesContent = "";
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line[0] == '[') {
            size_t space = line.find(' ');
            if (space != std::string::npos) {
                std::string tag = line.substr(1, space - 1);
                size_t firstQuote = line.find('"');
                size_t lastQuote = line.rfind('"');
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
                    std::string val = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    if (tag == "White") player1Name = val;
                    else if (tag == "Black") player2Name = val;
                    else if (tag == "Variant") {
                        activeVariant = val;
                    }
                    else if (tag == "InitialKingCol") {
                        board.initialKingCol = std::stoi(val);
                    }
                    else if (tag == "InitialRookLCol") {
                        board.initialRookLCol = std::stoi(val);
                    }
                    else if (tag == "InitialRookRCol") {
                        board.initialRookRCol = std::stoi(val);
                    }
                    else if (tag == "FEN") {
                        setupBoardFromFEN(board, val);
                        stateHistory[0] = board;
                    }
                    else if (tag == "WhiteTime") {
                        try { loadedWhiteTime = std::stof(val); } catch(...) {}
                    }
                    else if (tag == "BlackTime") {
                        try { loadedBlackTime = std::stof(val); } catch(...) {}
                    }
                    else if (tag == "TimeControl") {
                        try { loadedTimeControl = std::stof(val); } catch(...) {}
                    }
                    else if (tag == "IncrementTime") {
                        try { incrementTime = std::stof(val); } catch(...) {}
                    }
                    else if (tag == "DelayTime") {
                        try { delayTime = std::stof(val); } catch(...) {}
                    }
                    else if (tag == "ReviewPly") {
                        try { loadedReviewPly = std::stoi(val); } catch(...) {}
                    }
                }
            }
        } else {
            movesContent += line + " ";
        }
    }

    if (loadedTimeControl > 0.f) initialTimeLimit = loadedTimeControl;
    isUnlimitedTime = (loadedTimeControl > 9999.f || loadedTimeControl <= 0.f); // check if unlimited
    whiteTime = (loadedWhiteTime >= 0.f) ? loadedWhiteTime : initialTimeLimit;
    blackTime = (loadedBlackTime >= 0.f) ? loadedBlackTime : initialTimeLimit;

    std::stringstream ss(movesContent);
    std::string token;
    
    lastMoveFrom = { -1, -1 };
    lastMoveTo   = { -1, -1 };

    while (ss >> token) {
        if (token.find('.') != std::string::npos) continue;
        if (token == "*" || token == "1-0" || token == "0-1" || token == "1/2-1/2") continue;
        if (token.empty()) continue;
        
        int fromR = -1, fromC = -1, toR = -1, toC = -1;
        char promoChar = '\0';
        
        if (token.find('@') != std::string::npos) {
            // Drop notation, e.g. N@e4 or P@f6
            char piece = token[0];
            if (!board.isWhiteTurn) {
                piece = std::tolower(piece);
            } else {
                piece = std::toupper(piece);
            }
            size_t atPos = token.find('@');
            if (atPos + 2 < token.size()) {
                toC = token[atPos + 1] - 'a';
                toR = 8 - (token[atPos + 2] - '0');
            }
            if (toR >= 0 && toR < 8 && toC >= 0 && toC < 8) {
                board.dropPiece(piece, toR, toC);
                moveList.push_back(token);
                stateHistory.push_back(board);
                currentMoveIndex++;
                lastMoveFrom = { -1, -1 };
                lastMoveTo = { toR, toC };
            }
            continue;
        }
        
        if (token == "O-O") {
            fromR = board.isWhiteTurn ? 7 : 0;
            fromC = board.initialKingCol;
            toR   = board.isWhiteTurn ? 7 : 0;
            toC   = board.initialRookRCol;
        } else if (token == "O-O-O") {
            fromR = board.isWhiteTurn ? 7 : 0;
            fromC = board.initialKingCol;
            toR   = board.isWhiteTurn ? 7 : 0;
            toC   = board.initialRookLCol;
        } else {
            int start = std::isupper(token[0]) ? 1 : 0;
            if (start + 4 < (int)token.size()) {
                fromC = token[start] - 'a';
                fromR = 8 - (token[start + 1] - '0');
                toC   = token[start + 3] - 'a';
                toR   = 8 - (token[start + 4] - '0');
                
                size_t eq = token.find('=');
                if (eq != std::string::npos && eq + 1 < token.size()) {
                    promoChar = token[eq + 1];
                }
            }
        }
        
        if (fromR >= 0 && fromR < 8 && fromC >= 0 && fromC < 8 &&
            toR >= 0 && toR < 8 && toC >= 0 && toC < 8) {
            
            bool needsPromo = board.movePiece(fromR, fromC, toR, toC);
            if (needsPromo && promoChar != '\0') {
                board.promotePiece(toR, toC, promoChar);
            }
            
            moveList.push_back(token);
            stateHistory.push_back(board);
            currentMoveIndex++;
            
            lastMoveFrom = { fromR, fromC };
            lastMoveTo   = { toR, toC };
        }
    }

    currentPGNPath = path;
    if (loadedReviewPly >= 0 && !stateHistory.empty()) {
        currentMoveIndex = std::max(0, std::min(loadedReviewPly, (int)stateHistory.size() - 1));
        board = stateHistory[currentMoveIndex];
        updateLastMoveHighlight();
    }
    isTimerRunning = false;
    bool inCheckmate = board.isCheckmate(board.isWhiteTurn);
    bool inStalemate = board.isStalemate(board.isWhiteTurn);
    bool isDraw = inStalemate || board.halfmoveClock >= 100 || checkThreefoldRepetition() || board.isInsufficientMaterial();

    bool isWhiteKingInCenter = false;
    bool isBlackKingInCenter = false;
    for (int r = 3; r <= 4; ++r) {
        for (int c = 3; c <= 4; ++c) {
            if (board.board[r][c] == 'K') isWhiteKingInCenter = true;
            if (board.board[r][c] == 'k') isBlackKingInCenter = true;
        }
    }
    bool kingOfHillWin = (activeVariant == "King of the Hill" && (isWhiteKingInCenter || isBlackKingInCenter));

    bool threeCheckWin = (activeVariant == "3-Check" && (board.checksDeliveredByWhite >= 3 || board.checksDeliveredByBlack >= 3));

    isGameFinished = inCheckmate || isDraw || 
                     (whiteTime <= 0.f && !isUnlimitedTime) || 
                     (blackTime <= 0.f && !isUnlimitedTime) ||
                     kingOfHillWin ||
                     threeCheckWin;
    isReviewing = false;
    endGamePulse = 0.f;
}

void Game::renderLoadGameScreen() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    // Draw background texture
    window.clear(sf::Color(20, 16, 5));
    if (menu && menu->isFadingIn()) { // Wait, let's just use simple draw background if loaded
        // Render background sprite or solid fill
    }
    
    // Dimmed calm background
    if (gameBgLoaded) {
        float sx = W / gameBgTexture.getSize().x;
        float sy = H / gameBgTexture.getSize().y;
        gameBgSprite.setScale(sx, sy);
        window.draw(gameBgSprite);
    }
    sf::RectangleShape bgOverlay(sf::Vector2f(W, H));
    bgOverlay.setFillColor(sf::Color(15, 15, 20, 230));
    window.draw(bgOverlay);

    // Decorative ambient circles
    sf::CircleShape glow(H * 0.35f);
    glow.setFillColor(sf::Color(180, 145, 40, 6));
    glow.setOrigin(glow.getRadius(), glow.getRadius());
    glow.setPosition(W * 0.5f, H * 0.45f);
    window.draw(glow);

    // Title text
    std::string titleStr = "LOAD MATCH";
    float scaleTitle = 1.0f;
    float twTitle = stylishFont.getTextWidth(titleStr, scaleTitle);
    stylishFont.drawText(window, titleStr, sf::Vector2f(W * 0.5f - twTitle / 2.f, H * 0.08f), scaleTitle, sf::Color(220, 195, 120));

    // Divider line
    sf::RectangleShape div(sf::Vector2f(W * 0.5f, 1.5f));
    div.setPosition(W * 0.25f, H * 0.19f);
    div.setFillColor(sf::Color(180, 145, 40, 140));
    window.draw(div);

    // Mouse coordinates
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f mp = window.mapPixelToCoords(mousePos);

    // Display list
    if (savedMatches.empty()) {
        std::string noSave = "NO SAVED MATCHES FOUND";
        float sc = 0.22f;
        float tw = whiteFont.getTextWidth(noSave, sc);
        whiteFont.drawText(window, noSave, sf::Vector2f(W * 0.5f - tw / 2.f, H * 0.45f), sc, sf::Color(140, 140, 150, 180));
    } else {
        int nSaved = (int)savedMatches.size();
        int maxVisible = 4;
        int startIdx = 0;
        if (nSaved > maxVisible) {
            startIdx = selectedMatchIndex - maxVisible / 2;
            if (startIdx < 0) startIdx = 0;
            if (startIdx + maxVisible > nSaved) startIdx = nSaved - maxVisible;
        }
        int endIdx = std::min(nSaved, startIdx + maxVisible);

        float cardW = std::min(560.f, std::max(420.f, W * 0.48f));
        float cardH = 90.f;
        float gap = 18.f;
        float startY = H * 0.23f;

        for (int i = startIdx; i < endIdx; ++i) {
            float cardY = startY + (i - startIdx) * (cardH + gap);
            float cardX = W / 2.f - cardW / 2.f;

            sf::FloatRect cardRect(cardX, cardY, cardW, cardH);
            bool hovered = cardRect.contains(mp);
            bool isSel = (i == selectedMatchIndex);

            sf::Color cardFill = isSel ? sf::Color(32, 28, 20, 245) :
                                 hovered ? sf::Color(25, 25, 30, 240) :
                                           sf::Color(18, 18, 22, 210);
            sf::Color cardOutline = isSel ? sf::Color(210, 185, 60, 230) :
                                    hovered ? sf::Color(160, 160, 175, 160) :
                                              sf::Color(55, 55, 65, 120);
            float cardBorder = isSel ? 1.8f : hovered ? 1.5f : 1.2f;
            ui::drawRoundedRect(window, {cardX + 5.f, cardY + 6.f, cardW, cardH}, 7.f,
                                sf::Color(0, 0, 0, 65));
            ui::drawRoundedPanel(window, {cardX, cardY, cardW, cardH}, 7.f,
                                 cardFill, cardOutline, cardBorder);

            // Left side texts
            std::string playersStr = savedMatches[i].player1 + " vs " + savedMatches[i].player2;
            float scPlay = 0.18f;
            sf::Color colPlay = isSel ? sf::Color(240, 215, 140) : sf::Color(220, 220, 230);
            whiteFont.drawText(window, playersStr, sf::Vector2f(cardX + 22.f, cardY + 16.f), scPlay, colPlay);

            std::string dateStr = "Saved: " + savedMatches[i].date;
            float scDate = 0.12f;
            whiteFont.drawText(window, dateStr, sf::Vector2f(cardX + 22.f, cardY + 54.f), scDate, sf::Color(130, 130, 140));

            // Right side texts
            std::string movesStr = std::to_string(savedMatches[i].moveCount) + " Moves";
            float scMoves = 0.13f;
            whiteFont.drawText(window, movesStr, sf::Vector2f(cardX + cardW * 0.68f, cardY + 54.f), scMoves, sf::Color(180, 150, 90));

            // Result Text
            std::string statusStr = "In Progress";
            sf::Color colStatus = sf::Color(100, 200, 100);
            if (savedMatches[i].result == "1-0") {
                statusStr = "White Won";
                colStatus = sf::Color(230, 230, 230);
            } else if (savedMatches[i].result == "0-1") {
                statusStr = "Black Won";
                colStatus = sf::Color(130, 130, 140);
            } else if (savedMatches[i].result == "1/2-1/2") {
                statusStr = "Draw";
                colStatus = sf::Color(170, 170, 175);
            }
            float scStat = 0.16f;
            float twStat = whiteFont.getTextWidth(statusStr, scStat);
            whiteFont.drawText(window, statusStr, sf::Vector2f(cardX + cardW - twStat - 22.f, cardY + 16.f), scStat, colStatus);
        }
    }

    // BACK button
    float btnW = 200.f;
    float btnH = 42.f;
    float btnX = W / 2.f - btnW / 2.f;
    float btnY = H * 0.86f;

    sf::FloatRect btnRect(btnX, btnY, btnW, btnH);
    bool btnHovered = btnRect.contains(mp);

    ui::drawRoundedPanel(window, {btnX, btnY, btnW, btnH}, 5.f,
                         btnHovered ? sf::Color(173, 146, 29) : sf::Color(28, 28, 36),
                         btnHovered ? sf::Color(210, 185, 60) : sf::Color(80, 80, 90),
                         1.2f);

    std::string backLabel = "BACK";
    float scBack = 0.16f;
    float twBack = whiteFont.getTextWidth(backLabel, scBack);
    sf::Color backCol = btnHovered ? sf::Color(14, 14, 18) : sf::Color(200, 200, 215);
    whiteFont.drawText(window, backLabel, sf::Vector2f(btnX + btnW / 2.f - twBack / 2.f, btnY + btnH / 2.f - 7.f), scBack, backCol);
}

void Game::handleLoadGameEvents(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            startTransition(State::MENU);
            if (menu) menu->startFadeIn();
        }

        if (!savedMatches.empty()) {
            int n = (int)savedMatches.size();
            if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::W) {
                selectedMatchIndex = (selectedMatchIndex + n - 1) % n;
            }
            if (event.key.code == sf::Keyboard::Down || event.key.code == sf::Keyboard::S) {
                selectedMatchIndex = (selectedMatchIndex + 1) % n;
            }
            if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Space) {
                loadGameFromPGN(savedMatches[selectedMatchIndex].filePath);
                startTransition(State::PLAYING);
            }
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mp = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});

        // Check BACK button click
        float W = (float)window.getSize().x;
        float H = (float)window.getSize().y;
        float btnW = 200.f;
        float btnH = 42.f;
        float btnX = W / 2.f - btnW / 2.f;
        float btnY = H * 0.86f;

        if (sf::FloatRect(btnX, btnY, btnW, btnH).contains(mp)) {
            startTransition(State::MENU);
            if (menu) menu->startFadeIn();
            return;
        }

        // Check card clicks
        if (!savedMatches.empty()) {
            int nSaved = (int)savedMatches.size();
            int maxVisible = 4;
            int startIdx = 0;
            if (nSaved > maxVisible) {
                startIdx = selectedMatchIndex - maxVisible / 2;
                if (startIdx < 0) startIdx = 0;
                if (startIdx + maxVisible > nSaved) startIdx = nSaved - maxVisible;
            }
            int endIdx = std::min(nSaved, startIdx + maxVisible);

            float cardW = std::min(560.f, std::max(420.f, W * 0.48f));
            float cardH = 90.f;
            float gap = 18.f;
            float startY = H * 0.23f;

            for (int i = startIdx; i < endIdx; ++i) {
                float cardY = startY + (i - startIdx) * (cardH + gap);
                float cardX = W / 2.f - cardW / 2.f;

                if (sf::FloatRect(cardX, cardY, cardW, cardH).contains(mp)) {
                    if (i == selectedMatchIndex) {
                        // Double click / direct action: load!
                        loadGameFromPGN(savedMatches[i].filePath);
                        startTransition(State::PLAYING);
                    } else {
                        // Select it
                        selectedMatchIndex = i;
                    }
                    break;
                }
            }
        }
    }
}


// ─── updateLastMoveHighlight ───────────────────────────────────────────────
// Re-derives lastMoveFrom/lastMoveTo from moveList on Undo/Redo so the
// olive-yellow highlight always follows the correct preceding move.
void Game::updateLastMoveHighlight() {
    if (currentMoveIndex <= 0 || moveList.empty()) {
        lastMoveFrom = lastMoveTo = { -1, -1 };
        return;
    }
    const std::string& mv = moveList[currentMoveIndex - 1];
    bool movedByWhite = !board.isWhiteTurn;

    if (mv == "O-O") {
        int r = movedByWhite ? 7 : 0;
        lastMoveFrom = { r, 4 }; lastMoveTo = { r, 6 };
        return;
    }
    if (mv == "O-O-O") {
        int r = movedByWhite ? 7 : 0;
        lastMoveFrom = { r, 4 }; lastMoveTo = { r, 2 };
        return;
    }
    int st = std::isupper((unsigned char)mv[0]) ? 1 : 0;
    if ((int)mv.size() < st + 5) {
        lastMoveFrom = lastMoveTo = { -1, -1 };
        return;
    }
    char fc = mv[st];     char fr = mv[st + 1];
    char tc = mv[st + 3]; char tr = mv[st + 4];
    if (fc < 'a' || fc > 'h' || fr < '1' || fr > '8' ||
        tc < 'a' || tc > 'h' || tr < '1' || tr > '8') {
        lastMoveFrom = lastMoveTo = { -1, -1 };
        return;
    }
    lastMoveFrom = { 8 - (fr - '0'), fc - 'a' };
    lastMoveTo   = { 8 - (tr - '0'), tc - 'a' };
}

void Game::renderEndGameModal() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    bool inCheckmate = board.isCheckmate(board.isWhiteTurn);
    bool inStalemate = board.isStalemate(board.isWhiteTurn);
    bool flagWhite   = (whiteTime <= 0.f);
    bool flagBlack   = (blackTime <= 0.f);
    bool is50Moves   = (board.halfmoveClock >= 100);
    bool isRepetition = checkThreefoldRepetition();
    bool isInsufficient = board.isInsufficientMaterial();

    bool isWhiteKingInCenter = false;
    bool isBlackKingInCenter = false;
    for (int r = 3; r <= 4; ++r) {
        for (int c = 3; c <= 4; ++c) {
            if (board.board[r][c] == 'K') isWhiteKingInCenter = true;
            if (board.board[r][c] == 'k') isBlackKingInCenter = true;
        }
    }
    bool kingOfHillWhiteWin = (activeVariant == "King of the Hill" && isWhiteKingInCenter);
    bool kingOfHillBlackWin = (activeVariant == "King of the Hill" && isBlackKingInCenter);
    bool kingOfHillWin = kingOfHillWhiteWin || kingOfHillBlackWin;

    bool threeCheckWhiteWin = (activeVariant == "3-Check" && board.checksDeliveredByWhite >= 3);
    bool threeCheckBlackWin = (activeVariant == "3-Check" && board.checksDeliveredByBlack >= 3);
    bool threeCheckWin = threeCheckWhiteWin || threeCheckBlackWin;

    std::string headline, subtext;
    sf::Color   subColor, headColor;

    if (inCheckmate) {
        bool blackWon = board.isWhiteTurn;
        headline  = blackWon ? "BLACK WINS!" : "WHITE WINS!";
        subtext   = "BY CHECKMATE";
        subColor  = blackWon ? sf::Color(175,175,195) : sf::Color(245,235,200);
        headColor = blackWon ? sf::Color(185,185,210) : sf::Color(255,240,180);
    } else if (kingOfHillWin) {
        headline  = kingOfHillWhiteWin ? "WHITE WINS!" : "BLACK WINS!";
        subtext   = "KING OF THE HILL (CENTER REACHED)";
        subColor  = kingOfHillWhiteWin ? sf::Color(245,235,200) : sf::Color(175,175,195);
        headColor = kingOfHillWhiteWin ? sf::Color(255,240,180) : sf::Color(185,185,210);
    } else if (threeCheckWin) {
        headline  = threeCheckWhiteWin ? "WHITE WINS!" : "BLACK WINS!";
        subtext   = "BY 3-CHECK (3 CHECKS DELIVERED)";
        subColor  = threeCheckWhiteWin ? sf::Color(245,235,200) : sf::Color(175,175,195);
        headColor = threeCheckWhiteWin ? sf::Color(255,240,180) : sf::Color(185,185,210);
    } else if (inStalemate) {
        headline  = "DRAW";
        subtext   = "STALEMATE";
        subColor  = sf::Color(185,175,220);
        headColor = sf::Color(200,190,235);
    } else if (is50Moves) {
        headline  = "DRAW";
        subtext   = "50-MOVE RULE";
        subColor  = sf::Color(185,175,220);
        headColor = sf::Color(200,190,235);
    } else if (isRepetition) {
        headline  = "DRAW";
        subtext   = "THREEFOLD REPETITION";
        subColor  = sf::Color(185,175,220);
        headColor = sf::Color(200,190,235);
    } else if (isInsufficient) {
        headline  = "DRAW";
        subtext   = "INSUFFICIENT MATERIAL";
        subColor  = sf::Color(185,175,220);
        headColor = sf::Color(200,190,235);
    } else if (flagWhite && !isUnlimitedTime) {
        headline  = "BLACK WINS!";
        subtext   = "WHITE RAN OUT OF TIME";
        subColor  = sf::Color(175,175,195);
        headColor = sf::Color(185,185,210);
    } else if (flagBlack && !isUnlimitedTime) {
        headline  = "WHITE WINS!";
        subtext   = "BLACK RAN OUT OF TIME";
        subColor  = sf::Color(245,235,200);
        headColor = sf::Color(255,240,180);
    } else {
        headline  = "GAME OVER";
        subtext   = "";
        subColor  = sf::Color(200,200,200);
        headColor = sf::Color(220,195,120);
    }

    // ── REVIEW MODE: floating "VIEW RESULT" button ───────────────────────
    if (isReviewing) {
        float rbW = 152.f, rbH = 40.f;
        float rbX = W - rbW - 22.f;
        float rbY = H - rbH - 22.f;
        sf::Vector2i mi = sf::Mouse::getPosition(window);
        sf::Vector2f mp((float)mi.x, (float)mi.y);
        bool hov = sf::FloatRect(rbX, rbY, rbW, rbH).contains(mp);
        ui::drawRoundedRect(window, {rbX + 4.f, rbY + 4.f, rbW, rbH}, 5.f,
                            sf::Color(0,0,0,90));
        ui::drawRoundedPanel(window, {rbX, rbY, rbW, rbH}, 5.f,
                             hov ? sf::Color(173,146,29) : sf::Color(22,22,28,235),
                             hov ? sf::Color(220,195,90) : sf::Color(130,110,50,200),
                             1.5f);
        std::string rl = "VIEW RESULT";
        float rs = 0.14f;
        float rw = whiteFont.getTextWidth(rl, rs);
        whiteFont.drawText(window, rl,
            { rbX + rbW/2.f - rw/2.f, rbY + rbH/2.f - 7.f },
            rs, hov ? sf::Color(14,14,18) : sf::Color(210,185,60));
        return;
    }

    // ── Screen backdrop ──────────────────────────────────────────────────
    sf::RectangleShape backdrop(sf::Vector2f(W, H));
    float backdropAlpha = std::min(195.f, (endGamePulse / 2.0f) * 195.f);
    backdrop.setFillColor(sf::Color(10, 10, 16, (sf::Uint8)backdropAlpha));
    window.draw(backdrop);

    // If we are still in the 2-second delay, return after drawing the backdrop!
    if (endGamePulse < 2.0f) {
        return;
    }

    // Compute bubbly scale factor
    float scale = 1.f;
    float animT = endGamePulse - 2.0f;
    if (animT < 0.4f) {
        float ap = animT / 0.4f;
        float ts = ap - 1.f;
        scale = 1.f + 2.7f * ts * ts * ts + 1.7f * ts * ts;
    }

    sf::Transform trans;
    trans.translate(W / 2.f, H / 2.f);
    trans.scale(scale, scale);
    trans.translate(-W / 2.f, -H / 2.f);

    // ── Card ─────────────────────────────────────────────────────────────
    float cardW = 560.f, cardH = 480.f;
    float cardX = W/2.f - cardW/2.f;
    float cardY = H/2.f - cardH/2.f;
    ui::drawRoundedRect(window, {cardX+12.f, cardY+14.f, cardW, cardH}, 8.f,
                        sf::Color(0,0,0,100), trans);
    ui::drawRoundedPanel(window, {cardX, cardY, cardW, cardH}, 8.f,
                         sf::Color(20,20,26,248),
                         sf::Color(130,110,40,200),
                         1.8f,
                         trans);
    sf::RectangleShape topBar(sf::Vector2f(cardW-2.f, 4.f));
    topBar.setPosition(cardX+1.f, cardY+1.f);
    topBar.setFillColor(sf::Color(173,146,29,220));
    window.draw(topBar, trans);
    sf::CircleShape glow(cardW*0.58f);
    glow.setFillColor(sf::Color(160,130,30,9));
    glow.setOrigin(glow.getRadius(), glow.getRadius());
    glow.setPosition(W/2.f, cardY+cardH*0.28f);
    window.draw(glow, trans);

    // ── Headline ─────────────────────────────────────────────────────────
    {
        float hs = 0.38f;
        float hw = whiteFont.getTextWidth(headline, hs);
        whiteFont.drawText(window, headline,
            { W/2.f - hw/2.f, cardY+38.f }, hs, headColor, trans);
    }
    if (!subtext.empty()) {
        float ss2 = 0.17f;
        float sw  = whiteFont.getTextWidth(subtext, ss2);
        whiteFont.drawText(window, subtext,
            { W/2.f - sw/2.f, cardY+118.f }, ss2, subColor, trans);
    }
    sf::RectangleShape div1(sf::Vector2f(cardW-80.f, 1.f));
    div1.setPosition(cardX+40.f, cardY+150.f);
    div1.setFillColor(sf::Color(80,80,90,160));
    window.draw(div1, trans);

    // ── Stats ─────────────────────────────────────────────────────────────
    float statsY = cardY+164.f, lblSc = 0.12f, valSc = 0.19f;
    float col1X  = cardX+48.f,  col2X = cardX+cardW/2.f+22.f, rowH2 = 54.f;
    auto drawStat = [&](float sx, float sy,
                        const std::string& lbl, const std::string& val,
                        sf::Color vc) {
        whiteFont.drawText(window, lbl, {sx, sy},        lblSc, sf::Color(115,115,130), trans);
        whiteFont.drawText(window, val, {sx, sy+18.f},   valSc, vc, trans);
    };
    drawStat(col1X, statsY, "MOVES PLAYED", std::to_string(currentMoveIndex), sf::Color(220,215,178));
    {
        int wm=(int)(whiteTime/60.f), ws=(int)whiteTime%60;
        drawStat(col2X, statsY, "WHITE CLOCK",
            std::to_string(wm)+":"+(ws<10?"0":"")+std::to_string(ws), sf::Color(245,235,198));
    }
    {
        int bm=(int)(blackTime/60.f), bs=(int)blackTime%60;
        drawStat(col1X, statsY+rowH2, "BLACK CLOCK",
            std::to_string(bm)+":"+(bs<10?"0":"")+std::to_string(bs), sf::Color(170,170,184));
    }
    drawStat(col2X, statsY+rowH2, "WHITE PLAYER", player1Name, sf::Color(245,235,198));
    sf::RectangleShape div2(sf::Vector2f(cardW-80.f,1.f));
    div2.setPosition(cardX+40.f, statsY+rowH2*2.f+4.f);
    div2.setFillColor(sf::Color(80,80,90,160));
    window.draw(div2, trans);

    // ── Lifetime record ───────────────────────────────────────────────────
    float ltY = statsY+rowH2*2.f+18.f;
    std::string ltLbl = "LIFETIME RECORD";
    float ltLs = 0.12f, ltLw = whiteFont.getTextWidth(ltLbl, ltLs);
    whiteFont.drawText(window, ltLbl, {W/2.f-ltLw/2.f, ltY}, ltLs, sf::Color(100,100,115), trans);
    float ltValY=ltY+18.f, lvSc=0.22f, colW3=cardW/3.f, subLS=0.10f;
    auto drawLt = [&](float cx3, const std::string& val,
                      const std::string& lbl, sf::Color vc) {
        float vw=whiteFont.getTextWidth(val,lvSc);
        whiteFont.drawText(window, val,  {cx3-vw/2.f, ltValY},      lvSc, vc, trans);
        float lw2=whiteFont.getTextWidth(lbl,subLS);
        whiteFont.drawText(window, lbl,  {cx3-lw2/2.f, ltValY+28.f},subLS, sf::Color(115,115,128), trans);
    };
    drawLt(cardX+colW3*0.5f, std::to_string(statWhiteWins), "WHITE WINS", sf::Color(245,235,198));
    drawLt(cardX+colW3*1.5f, std::to_string(statDraws),     "DRAWS",      sf::Color(185,175,220));
    drawLt(cardX+colW3*2.5f, std::to_string(statBlackWins), "BLACK WINS", sf::Color(170,170,184));

    // ── Buttons ───────────────────────────────────────────────────────────
    const char* btnLbls[] = { "RETRY", "REVIEW", "MAIN MENU", "EXIT" };
    float bW2=110.f, bH2=42.f, bGap=14.f;
    float bRowX=cardX+(cardW-4.f*bW2-3.f*bGap)/2.f;
    float bRowY=cardY+cardH-68.f;
    sf::Vector2i mi2=sf::Mouse::getPosition(window);
    sf::Vector2f mp2((float)mi2.x,(float)mi2.y);
    sf::Color fN[]={sf::Color(65,100,20),sf::Color(22,55,95),sf::Color(28,28,38),sf::Color(28,28,38)};
    sf::Color fH[]={sf::Color(110,160,35),sf::Color(38,95,160),sf::Color(173,146,29),sf::Color(140,38,38)};
    sf::Color oN[]={sf::Color(105,155,32),sf::Color(48,105,170),sf::Color(70,70,84),sf::Color(70,70,84)};
    sf::Color oH[]={sf::Color(160,220,55),sf::Color(80,155,220),sf::Color(210,185,60),sf::Color(200,75,75)};
    sf::Color lN[]={sf::Color(155,210,100),sf::Color(120,185,240),sf::Color(195,195,210),sf::Color(195,195,210)};
    sf::Color lH[]={sf::Color(235,255,210),sf::Color(205,230,255),sf::Color(14,14,18),sf::Color(255,205,205)};
    for (int b=0; b<4; ++b) {
        float bX=bRowX+b*(bW2+bGap);
        bool hov=sf::FloatRect(bX,bRowY,bW2,bH2).contains(mp2);
        ui::drawRoundedRect(window, {bX+3.f,bRowY+3.f,bW2,bH2}, 5.f,
                            sf::Color(0,0,0,70), trans);
        ui::drawRoundedPanel(window, {bX,bRowY,bW2,bH2}, 5.f,
                             hov?fH[b]:fN[b],
                             hov?oH[b]:oN[b],
                             1.5f,
                             trans);
        float blw=whiteFont.getTextWidth(btnLbls[b],0.14f);
        whiteFont.drawText(window, btnLbls[b],
            {bX+bW2/2.f-blw/2.f, bRowY+bH2/2.f-7.f},
            0.14f, hov?lH[b]:lN[b], trans);
    }
}

bool Game::isBoardFlipped() const {
    std::string perspective = activePerspective;
    if (currentState == State::SETTINGS && settingsScreen) {
        perspective = settingsScreen->getBoardPerspective();
    }
    if (perspective == "Black") return true;
    if (perspective == "Auto") return !board.isWhiteTurn;
    return false;
}

sf::FloatRect Game::getPocketSlotRect(bool isWhitePocket, int index, float offsetX, float offsetY, float boardSize) const {
    float slotSize = 60.f;
    float gap = 12.f;
    float totalW = 5.f * slotSize + 4.f * gap;
    float startX = offsetX + (boardSize - totalW) / 2.f;
    
    bool isBottom = (isWhitePocket != isBoardFlipped());
    float y = isBottom ? (offsetY + boardSize + 40.f) : (offsetY - 80.f);
    
    return sf::FloatRect(startX + index * (slotSize + gap), y, slotSize, slotSize);
}

sf::Vector2f Game::getSquareScreenPos(int r, int c) const {
    bool flipped = isBoardFlipped();
    int displayR = flipped ? 7 - r : r;
    int displayC = flipped ? 7 - c : c;
    float boardSize = 8.0f * TILE;
    float offsetX = (window.getSize().x - boardSize) / 2.0f;
    float offsetY = (window.getSize().y - boardSize) / 2.0f;
    return { offsetX + displayC * TILE, offsetY + displayR * TILE };
}

bool Game::checkThreefoldRepetition() const {
    if (currentMoveIndex < 0 || currentMoveIndex >= (int)stateHistory.size()) return false;
    const ChessBoard& current = stateHistory[currentMoveIndex];
    int count = 0;
    for (int i = 0; i <= currentMoveIndex; ++i) {
        if (stateHistory[i].isSamePosition(current)) {
            count++;
        }
    }
    return count >= 3;
}
