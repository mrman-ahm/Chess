#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <vector>
#include "BitmapFont.hpp"

enum class SettingsAction { None, Back };

// A single "← value →" selector row
struct SelectorRow {
    std::string label;
    std::vector<std::string> options;
    int currentIndex = 0;

    // Arrow button animation state (left / right)
    float leftScale  = 1.0f;
    float rightScale = 1.0f;
    float focusAmount = 0.0f;
    float valuePulse = 0.0f;

    const std::string& current() const { return options[currentIndex]; }
    void prev() { if (currentIndex > 0) --currentIndex; else currentIndex = (int)options.size() - 1; }
    void next() { currentIndex = (currentIndex + 1) % (int)options.size(); }
};

struct PieceSetInfo {
    std::string displayName;
    std::string directory;
    std::map<char, std::string> files;
    std::map<char, sf::Texture> textures;
};

class SettingsScreen {
public:
    SettingsScreen(sf::RenderWindow& win, BitmapFont& title, BitmapFont& body);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw();

    SettingsAction getAction() const { return pendingAction; }
    void clearAction()              { pendingAction = SettingsAction::None; }
    bool consumeChangeSound();
    bool consumeClickSound();

    // Readable by Game / other systems to apply chosen theme
    const std::string& getMenuPreset()       const { return rows[0].current(); }
    const std::string& getMenuBg()           const { return rows[0].current(); }
    const std::string& getDisplayMode()      const { return rows[1].current(); }
    const std::string& getBoardTileTheme()   const { return rows[2].current(); }
    const std::string& getBoardBg()          const { return rows[3].current(); }
    std::string getBoardBgPath() const;
    const std::map<std::string, std::string>& getBoardBackgroundFiles() const { return boardBackgroundFiles; }
    const std::string& getBoardPerspective() const { return rows[4].current(); }
    const std::string& getCursorStyle()      const { return rows[5].current(); }
    std::string getCursorPath() const;
    bool getFahhMode() const { return rows.size() > 6 && rows[6].current() == "On"; }
    bool getMenuSoundsEnabled() const { return rows.size() <= 7 || rows[7].current() == "On"; }
    bool getGameSoundsEnabled() const { return rows.size() <= 8 || rows[8].current() == "On"; }
    void setSelection(int rowIdx, const std::string& val);

    std::string getPieceMode() const;
    std::string getPieceSetName() const;
    std::string getCustomPieceSetName(char pieceType) const;
    std::string getPieceTexturePath(char boardPiece) const;
    void setPieceMode(const std::string& mode);
    void setPieceSetName(const std::string& name);
    void setCustomPieceSetName(char pieceType, const std::string& name);
    bool consumePieceSpritesChanged();

private:
    void buildRows();
    void scanBoardBackgrounds();
    void scanCursorStyles();
    void scanPieceSets();
    void drawBackground();
    void drawRow(SelectorRow& row, float cx, float y, bool focused);
    void drawArrow(float cx, float cy, bool pointRight, float scale, bool hovered);
    void drawButton(const std::string& label, sf::FloatRect rect, float hoverAmount);
    void drawLargeSelectionPreview();
    void drawPieceModal();
    void handlePieceModalEvent(const sf::Event& event);
    sf::FloatRect getPieceButtonRect() const;
    void openPieceModal();
    void applyPieceDraft();
    void cycleDraftSet(int delta);
    void cycleDraftCustom(char pieceType, int delta);
    int findPieceSetIndex(const std::string& name) const;
    int getAppliedSetIndexForPiece(char pieceType) const;

    sf::RenderWindow& window;
    BitmapFont&       titleFont;
    BitmapFont&       bodyFont;

    std::vector<SelectorRow> rows;
    int focusedRow = 0;   // keyboard navigation
    std::map<std::string, std::string> boardBackgroundFiles;
    std::map<std::string, sf::Texture> boardBackgroundTextures;
    std::map<std::string, std::string> cursorFiles;

    SettingsAction pendingAction = SettingsAction::None;
    bool changeSoundRequested = false;
    bool clickSoundRequested = false;

    float backHover = 0.f;
    float pieceButtonHover = 0.f;
    float animTimer = 0.f;  // global timer for subtle idle pulse

    std::vector<PieceSetInfo> pieceSets;
    bool pieceModalOpen = false;
    bool pieceSpritesChanged = false;
    bool appliedCustomizePieces = false;
    bool draftCustomizePieces = false;
    int appliedSetIndex = 0;
    int draftSetIndex = 0;
    std::map<char, int> appliedCustomSetIndex;
    std::map<char, int> draftCustomSetIndex;
};
