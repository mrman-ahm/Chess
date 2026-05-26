#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <array>
#include "BitmapFont.hpp"

// Signals this screen can emit back to Game
enum class SetupAction { None, StartMatch, Back };

// A self-contained player name text-field with a blinking cursor
struct NameInputField {
    std::string text;
    bool active      = false;   // Keyboard focus
    float cursorTimer = 0.0f;
    bool cursorVisible = true;

    void update(float dt) {
        cursorTimer += dt;
        if (cursorTimer >= 0.53f) { cursorVisible = !cursorVisible; cursorTimer = 0.f; }
    }
    void handleChar(char c) {
        if (c == '\b') { if (!text.empty()) text.pop_back(); }
        else if (text.size() < 16 && c >= 32 && c < 127) text += c;
    }
};

class PlayerSetupScreen {
public:
    PlayerSetupScreen(sf::RenderWindow& win, BitmapFont& title, BitmapFont& body);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw();

    SetupAction getAction() const { return pendingAction; }
    void clearAction()           { pendingAction = SetupAction::None; }

    // Retrieved by Game after StartMatch
    const std::string& getPlayer1Name() const { return fields[0].text; }
    const std::string& getPlayer2Name() const { return fields[1].text; }

    const std::string& getGameVariant() const {
        static const std::string standard = "Standard";
        return aiMode ? standard : rows[0].current();
    }
    const std::string& getBaseTime()    const { return rows[1].current(); }
    const std::string& getIncrement()   const { return rows[2].current(); }
    const std::string& getDelay()       const { return rows[3].current(); }
    const std::string& getPerspective() const { return rows[4].current(); }
    const std::string& getColorAssignment() const { return rows[5].current(); }
    const std::string& getAIDifficulty() const {
        static const std::string normal = "Normal";
        return aiMode ? rows[0].current() : normal;
    }

    void reset();   // Call each time the screen is opened
    void setAIMode(bool enabled);

private:
    struct SetupSelectorRow {
        std::string label;
        std::vector<std::string> options;
        int currentIndex = 0;
        float leftScale  = 1.0f;
        float rightScale = 1.0f;

        const std::string& current() const { return options[currentIndex]; }
        void prev() { if (currentIndex > 0) --currentIndex; else currentIndex = (int)options.size() - 1; }
        void next() { currentIndex = (currentIndex + 1) % (int)options.size(); }
    };

    void buildRows();
    void drawBackground();
    void drawPanel(float cx, float cy, const std::string& label,
                   NameInputField& field, int avatarSlot);
    void drawRow(SetupSelectorRow& row, float cx, float y, bool focused);
    void drawArrow(float cx, float cy, bool pointRight, float scale, bool hovered);
    void drawButton(const std::string& label, sf::FloatRect rect, bool hovered);
    void drawDivider(float cx, float cy, float halfH);

    sf::RenderWindow& window;
    BitmapFont&       titleFont;
    BitmapFont&       bodyFont;

    std::array<NameInputField, 2> fields; // [0]=Player1/White, [1]=Player2/Black
    int activeField = 0;                  // which field has focus (0=P1, 1=P2, 2=Settings)
    bool settingsFocused = false;         // whether the central settings column has keyboard focus
    bool aiMode = false;

    std::vector<SetupSelectorRow> rows;
    int focusedRow = 0;   // central settings navigation

    // Arrow-key avatar selection (placeholder indices, 0-3)
    std::array<int, 2> avatarIndex = {0, 0};

    SetupAction pendingAction = SetupAction::None;

    // Button hover timers for subtle scale animation
    float startHover  = 0.f;
    float backHover   = 0.f;
};
