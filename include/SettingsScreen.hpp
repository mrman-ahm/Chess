#pragma once
#include <SFML/Graphics.hpp>
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

class SettingsScreen {
public:
    SettingsScreen(sf::RenderWindow& win, BitmapFont& title, BitmapFont& body);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw();

    SettingsAction getAction() const { return pendingAction; }
    void clearAction()              { pendingAction = SettingsAction::None; }

    // Readable by Game / other systems to apply chosen theme
    const std::string& getMenuBg()           const { return rows[0].current(); }
    const std::string& getBoardTileTheme()   const { return rows[1].current(); }
    const std::string& getBoardBg()          const { return rows[2].current(); }
    const std::string& getBoardPerspective() const { return rows[3].current(); }
    const std::string& getCursorStyle()      const { return rows[4].current(); }
    void setSelection(int rowIdx, const std::string& val);

private:
    void buildRows();
    void drawBackground();
    void drawRow(SelectorRow& row, float cx, float y, bool focused);
    void drawArrow(float cx, float cy, bool pointRight, float scale, bool hovered);
    void drawButton(const std::string& label, sf::FloatRect rect, float hoverAmount);

    sf::RenderWindow& window;
    BitmapFont&       titleFont;
    BitmapFont&       bodyFont;

    std::vector<SelectorRow> rows;
    int focusedRow = 0;   // keyboard navigation

    SettingsAction pendingAction = SettingsAction::None;

    float backHover = 0.f;
    float animTimer = 0.f;  // global timer for subtle idle pulse
};
