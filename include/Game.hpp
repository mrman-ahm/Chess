#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "Chess.hpp"
#include "BitmapFont.hpp"
#include "Menu.hpp"
#include "PlayerSetupScreen.hpp"
#include "SettingsScreen.hpp"
#include "StockfishEngine.hpp"

class Game {
public:
    Game();
    void run();

private:
    enum class State { MENU, PLAYER_SETUP, SETTINGS, PLAYING, PROMOTING, LOAD_GAME };
    State currentState  = State::MENU;
    State previousState = State::MENU; // used by Settings to know where to return
    sf::Vector2i selectedSquare  = { -1, -1 };
    sf::Vector2i promotionCoord  = { -1, -1 };
    sf::Vector2i lastFromCoord   = { -1, -1 };
    std::string  tempPromoMovePrefix = "";
    char         selectedPocketPiece = '\0';
    std::string  activeVariant = "Standard";
    std::string  activePerspective = "White";
    bool         isUnlimitedTime = false;
    bool         pendingVsAIStart = false;

    // End-game state
    bool isReviewing    = false;  // Modal hidden for board review
    float endGamePulse  = 0.f;   // Timer for modal animation effects

    struct SavedMatchInfo {
        std::string filePath;
        std::string player1 = "Player 1";
        std::string player2 = "Player 2";
        std::string date = "Unknown Date";
        std::string result = "*";
        int moveCount = 0;
    };

    // Last-move highlight squares
    sf::Vector2i lastMoveFrom = { -1, -1 };
    sf::Vector2i lastMoveTo   = { -1, -1 };

    // Slide animation
    bool         isAnimating   = false;
    float        animProgress  = 0.f;
    float        animDuration  = 0.13f;
    char         animPiece     = '.';
    sf::Vector2f animFrom      = {0.f, 0.f};
    sf::Vector2f animTo        = {0.f, 0.f};

    // Drag-and-drop
    bool         isDragging        = false;
    sf::Vector2i dragSourceSquare  = { -1, -1 };
    sf::Vector2f dragCurrentPos    = {0.f, 0.f};

    // Screen transition fade
    float transAlpha    = 0.f;   // 0=transparent, 255=black
    bool  transOut      = false;  // true=fading to black, false=fading back in
    State transNextState = State::MENU;
    bool  sessionMenuOpen = false;
    float sessionMenuAnim = 0.f;
    float uiPulseTime = 0.f;
    bool  exitConfirmOpen = false;
    float exitConfirmAnim = 0.f;
    float exitYesHover = 0.f;
    float exitNoHover = 0.f;
    void  startTransition(State nextState);
    void  updateTransition(float dt);
    void  drawTransitionOverlay();

    void processEvents();
    void update(float dt);
    void render();
    void loadTextures();
    void loadCustomCursor();
    void applyCursorSetting();
    void renderGame();
    void renderExitConfirmation();
    void renderEndGameModal();         // Premium glassmorphic end-game popup
    void updateLastMoveHighlight();    // Re-derive highlights from moveList on undo/redo
    void drawPlayerPanel(float x, float y, const std::string& name, bool nameBelow);
    void renderGearButton();
    void renderActionButtons();
    void renderMoveHistoryPanel();
    bool handleMoveHistoryClick(const sf::Vector2f& mp);
    int getFirstVisibleHistoryPosition(int maxRows) const;
    void setReviewPosition(int positionIndex);
    void appendMoveToTimeline(const std::string& lan, const std::string& uci);
    void requestAIMoveIfNeeded();
    void executePendingAIMove();
    bool applyAIMove(const std::string& uciMove);
    bool chooseWeakAIMove(std::string& uciMove) const;
    std::string makeUCIMove(int fromRow, int fromCol, int toRow, int toCol, char promoChar = '\0') const;
    bool isHumanTurn() const;

    // Configuration & persistence systems
    void ensureDirectoriesExist();
    void loadConfig();
    void saveConfig();
    void saveCurrentGamePGN();
    std::string generatePGNContent(int limit);
    void drawPlayerClock(float x, float y, float timeRemaining, bool isActive);
    bool isBoardFlipped() const;
    sf::FloatRect getPocketSlotRect(bool isWhitePocket, int index, float offsetX, float offsetY, float boardSize) const;
    sf::Vector2f getSquareScreenPos(int r, int c) const;
    bool checkThreefoldRepetition() const;

    // Load Game screen and logic
    std::vector<SavedMatchInfo> savedMatches;
    int selectedMatchIndex = 0;
    void scanSavedMatches();
    void loadGameFromPGN(const std::string& path);
    void renderLoadGameScreen();
    void handleLoadGameEvents(const sf::Event& event);

    sf::RenderWindow window;
    sf::Cursor customCursor;
    sf::Cursor systemCursor;
    bool customCursorLoaded = false;
    bool systemCursorLoaded = false;
    bool usingCustomCursor = true;
    sf::Clock clock;

    ChessBoard board;
    BitmapFont blueFont;      // Used for menu buttons
    BitmapFont stylishFont;   // Used for the CHESS title
    BitmapFont whiteFont;     // Used for point representation
    std::map<char, sf::Texture> textures;
    sf::Texture whiteTurnTex;
    sf::Texture blackTurnTex;
    sf::Texture gameBgTexture;
    sf::Sprite gameBgSprite;
    bool gameBgLoaded = false;

    // Screens
    Menu*              menu        = nullptr;
    PlayerSetupScreen* setupScreen = nullptr;
    SettingsScreen*    settingsScreen = nullptr;

    // Player info populated by PlayerSetupScreen
    std::string player1Name = "Player 1";
    std::string player2Name = "Player 2";

    // Timeline variables for Undo/Redo
    std::vector<ChessBoard> stateHistory;
    std::vector<std::string> moveList;
    std::vector<std::string> uciMoveHistory;
    int currentMoveIndex = 0;

    // Local Stockfish AI
    StockfishEngine stockfish;
    bool isVsAI = false;
    bool aiPlaysWhite = false;
    bool aiMovePending = false;
    float aiMoveDelayRemaining = 0.f;
    float aiMoveDelaySeconds = 2.35f;
    int aiSearchDepth = 6;
    int aiSkillLevel = 5;

    // Save path
    std::string currentPGNPath;

    // Clock state
    float whiteTime = 600.f;
    float blackTime = 600.f;
    float initialTimeLimit = 600.f;
    float incrementTime = 0.f;
    float delayTime = 0.f;
    float activeDelayRemaining = 0.f;
    bool isTimerRunning = false;
    bool isGameFinished = false;

    // Stats
    int statWhiteWins = 0;
    int statBlackWins = 0;
    int statDraws = 0;
    int statTotalGames = 0;

    // Game board state
    const int TILE = 90;
    const sf::Color lightColor    = sf::Color(240, 217, 181);
    const sf::Color darkColor     = sf::Color(181, 136, 99);
    const sf::Color highlightColor = sf::Color(186, 202, 68);
};
