#include "PlayerSetupScreen.hpp"
#include "UIPrimitives.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

// ─── Palette ──────────────────────────────────────────────────────────────
static const sf::Color COL_BG_DARK  = sf::Color(14,  14,  18);
static const sf::Color COL_PANEL    = sf::Color(24,  24,  30);
static const sf::Color COL_BORDER   = sf::Color(70,  70,  82);
static const sf::Color COL_ACCENT   = sf::Color(173, 146, 29);   // #AD921D gold
static const sf::Color COL_ACCENT_L = sf::Color(210, 185, 60);
static const sf::Color COL_TEXT     = sf::Color(220, 220, 232);
static const sf::Color COL_SUBTEXT  = sf::Color(140, 140, 158);
static const sf::Color COL_CURSOR   = sf::Color(173, 146, 29, 220);
static const sf::Color COL_ACTIVE   = sf::Color(30,  30,  38);

PlayerSetupScreen::PlayerSetupScreen(sf::RenderWindow& win,
                                     BitmapFont& title,
                                     BitmapFont& body)
    : window(win), titleFont(title), bodyFont(body)
{
    fields[0].text = "Player 1";
    fields[1].text = "Player 2";
    fields[0].active = true;
    buildRows();
}

void PlayerSetupScreen::reset() {
    fields[0].text = "Player 1";
    fields[1].text = aiMode ? "Stockfish" : "Player 2";
    fields[0].active = true;
    fields[1].active = false;
    activeField = 0;
    settingsFocused = false;
    avatarIndex = {0, 0};
    pendingAction = SetupAction::None;
    startHover = backHover = 0.f;
    buildRows();
}

void PlayerSetupScreen::setAIMode(bool enabled) {
    aiMode = enabled;
    reset();
}

void PlayerSetupScreen::buildRows() {
    rows.clear();
    // Row 0: Game Variant
    {
        SetupSelectorRow r;
        if (aiMode) {
            r.label = "AI LEVEL";
            for (int i = 0; i <= 20; ++i) {
                r.options.push_back(std::to_string(i));
            }
            r.currentIndex = 5; // Gentle default, still above pure-random beginner play
        } else {
            r.label = "GAME VARIANT";
            r.options = { "Standard", "Chess 960", "Crazyhouse", "3-Check", "King of the Hill" };
            r.currentIndex = 0;
        }
        rows.push_back(r);
    }
    // Row 1: Time Control
    {
        SetupSelectorRow r;
        r.label = "TIME CONTROL";
        r.options = { "Unlimited", "1 Min", "3 Min", "5 Min", "10 Min", "15 Min", "30 Min", "60 Min" };
        r.currentIndex = 4; // Default 10 Min
        rows.push_back(r);
    }
    // Row 2: Increment
    {
        SetupSelectorRow r;
        r.label = "INCREMENT";
        r.options = { "0 Sec", "1 Sec", "2 Sec", "3 Sec", "5 Sec", "10 Sec", "15 Sec", "30 Sec" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // Row 3: Delay
    {
        SetupSelectorRow r;
        r.label = "DELAY";
        r.options = { "0 Sec", "1 Sec", "2 Sec", "3 Sec", "5 Sec", "10 Sec", "15 Sec", "30 Sec" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // Row 4: Perspective
    {
        SetupSelectorRow r;
        r.label = "BOARD PERSPECTIVE";
        r.options = { "White", "Black", "Auto" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // Row 5: Color Assignment
    {
        SetupSelectorRow r;
        r.label = aiMode ? "PLAY AS" : "COLOR ASSIGNMENT";
        r.options = aiMode ? std::vector<std::string>{ "White", "Black", "Random" }
                           : std::vector<std::string>{ "Default", "Random" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
}

// ─── Events ───────────────────────────────────────────────────────────────
void PlayerSetupScreen::handleEvent(const sf::Event& event) {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    if (event.type == sf::Event::TextEntered) {
        if (activeField < 2 && !(aiMode && activeField == 1)) {
            fields[activeField].handleChar((char)event.text.unicode);
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Tab) {
            if (activeField == 0) {
                fields[0].active = false;
                if (aiMode) {
                    activeField = 2;
                    settingsFocused = true;
                } else {
                    activeField = 1;
                    fields[1].active = true;
                    settingsFocused = false;
                }
            } else if (activeField == 1) {
                fields[1].active = false;
                activeField = 2; // Central settings
                settingsFocused = true;
            } else {
                activeField = 0;
                fields[0].active = true;
                settingsFocused = false;
            }
        }
        if (event.key.code == sf::Keyboard::Escape) {
            pendingAction = SetupAction::Back;
        }
        if (event.key.code == sf::Keyboard::Return) {
            if (!fields[0].text.empty() && !fields[1].text.empty())
                pendingAction = SetupAction::StartMatch;
        }
        
        if (settingsFocused) {
            if (event.key.code == sf::Keyboard::Up)
                focusedRow = std::max(0, focusedRow - 1);
            if (event.key.code == sf::Keyboard::Down)
                focusedRow = std::min((int)rows.size() - 1, focusedRow + 1);
            if (event.key.code == sf::Keyboard::Left) {
                rows[focusedRow].prev();
                rows[focusedRow].leftScale = 1.35f;
            }
            if (event.key.code == sf::Keyboard::Right) {
                rows[focusedRow].next();
                rows[focusedRow].rightScale = 1.35f;
            }
        } else {
            // Avatar cycling via arrow keys
            if (event.key.code == sf::Keyboard::Left && activeField < 2)
                avatarIndex[activeField] = (avatarIndex[activeField] + 3) % 4;
            if (event.key.code == sf::Keyboard::Right && activeField < 2)
                avatarIndex[activeField] = (avatarIndex[activeField] + 1) % 4;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mp(event.mouseButton.x, event.mouseButton.y);

        float p1x = W * 0.04f;
        float cx  = W * 0.32f;
        float p2x = W * 0.72f;
        float py  = H * 0.20f;
        float pW  = W * 0.24f;
        float cW  = W * 0.36f;
        float pH  = H * 0.56f;

        // Player panel click-to-focus
        if (sf::FloatRect(p1x, py, pW, pH).contains(mp)) {
            fields[0].active = true;  fields[1].active = false; activeField = 0; settingsFocused = false;
        } else if (!aiMode && sf::FloatRect(p2x, py, pW, pH).contains(mp)) {
            fields[1].active = true;  fields[0].active = false; activeField = 1; settingsFocused = false;
        } else if (sf::FloatRect(cx, py, cW, pH).contains(mp)) {
            fields[0].active = false; fields[1].active = false; activeField = 2; settingsFocused = true;
            
            // Check settings row arrows click
            float rowH   = 52.f;
            float rowGap = 10.f;
            float startY = py + 48.f;
            float rowCX  = cx + cW / 2.f;
            
            for (int i = 0; i < (int)rows.size(); ++i) {
                float rowY = startY + i * (rowH + rowGap);
                sf::FloatRect leftRect(rowCX - 130.f, rowY + 6.f, 30.f, 30.f);
                sf::FloatRect rightRect(rowCX + 100.f, rowY + 6.f, 30.f, 30.f);
                
                if (leftRect.contains(mp)) {
                    focusedRow = i;
                    rows[i].prev();
                    rows[i].leftScale = 1.35f;
                } else if (rightRect.contains(mp)) {
                    focusedRow = i;
                    rows[i].next();
                    rows[i].rightScale = 1.35f;
                } else if (sf::FloatRect(cx, rowY, cW, rowH).contains(mp)) {
                    focusedRow = i;
                    if (aiMode && rows[i].label == "AI LEVEL") {
                        float sliderX = rowCX - 82.f;
                        float sliderW = 164.f;
                        float ratio = std::clamp((mp.x - sliderX) / sliderW, 0.f, 1.f);
                        rows[i].currentIndex = (int)std::round(ratio * ((int)rows[i].options.size() - 1));
                    }
                }
            }
        }

        // Start Match button
        float btnW = 200.f, btnH = 48.f;
        float btnX = W / 2.f - btnW / 2.f;
        float btnY = H * 0.84f;
        if (sf::FloatRect(btnX, btnY, btnW, btnH).contains(mp)) {
            if (!fields[0].text.empty() && !fields[1].text.empty())
                pendingAction = SetupAction::StartMatch;
        }

        // Back button
        if (sf::FloatRect(40.f, 30.f, 100.f, 36.f).contains(mp))
            pendingAction = SetupAction::Back;
    }
}

// ─── Update ───────────────────────────────────────────────────────────────
void PlayerSetupScreen::update(float dt) {
    fields[0].update(dt);
    fields[1].update(dt);

    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mp(mi);

    // Animate button hover scale
    float btnW = 200.f, btnH = 48.f;
    float btnX = W / 2.f - btnW / 2.f;
    float btnY = H * 0.84f;
    startHover += (sf::FloatRect(btnX, btnY, btnW, btnH).contains(mp) ? dt : -dt) * 4.f;
    startHover  = std::clamp(startHover, 0.f, 1.f);

    backHover += (sf::FloatRect(40.f, 30.f, 100.f, 36.f).contains(mp) ? dt : -dt) * 4.f;
    backHover  = std::clamp(backHover, 0.f, 1.f);

    // Decay bulge scale back to 1.0 smoothly
    for (auto& row : rows) {
        row.leftScale  += (1.0f - row.leftScale)  * dt * 12.f;
        row.rightScale += (1.0f - row.rightScale) * dt * 12.f;
    }
}

// ─── Draw helpers ─────────────────────────────────────────────────────────
void PlayerSetupScreen::drawBackground() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    sf::RectangleShape bg({W, H});
    bg.setFillColor(COL_BG_DARK);
    window.draw(bg);

    sf::RectangleShape glow({W * 0.65f, H * 0.65f});
    glow.setOrigin(glow.getSize() / 2.f);
    glow.setPosition(W / 2.f, H / 2.f);
    glow.setFillColor(sf::Color(173, 146, 29, 8));
    window.draw(glow);
}

void PlayerSetupScreen::drawPanel(float px, float py,
                                  const std::string& label,
                                  NameInputField& field,
                                  int avatarSlot) {
    float W  = (float)window.getSize().x;
    float H  = (float)window.getSize().y;
    float pW = W * 0.24f;
    float pH = H * 0.56f;
    bool  focused = field.active;

    ui::drawRoundedRect(window, {px + 8, py + 10, pW, pH}, 7.f,
                        sf::Color(0, 0, 0, 60));
    ui::drawRoundedPanel(window, {px, py, pW, pH}, 7.f,
                         COL_PANEL,
                         focused ? COL_ACCENT : COL_BORDER,
                         focused ? 1.5f : 1.0f);

    if (focused) {
        sf::RectangleShape bar({pW, 3.f});
        bar.setPosition(px, py);
        bar.setFillColor(COL_ACCENT);
        window.draw(bar);
    }

    float cx = px + pW / 2.f;
    float innerY = py + 28.f;

    float lScale = 0.20f;
    float lw     = bodyFont.getTextWidth(label, lScale);
    bodyFont.drawText(window, label,
                      {cx - lw / 2.f, innerY},
                      lScale, COL_SUBTEXT);
    innerY += 28.f;

    sf::RectangleShape sep({pW * 0.8f, 1.f});
    sep.setPosition(px + pW * 0.1f, innerY);
    sep.setFillColor(COL_BORDER);
    window.draw(sep);
    innerY += 14.f;

    float avSize = pW * 0.44f;
    float avX    = cx - avSize / 2.f;

    ui::drawRoundedRect(window, {avX + 4, innerY + 4, avSize, avSize}, 6.f,
                        sf::Color(0, 0, 0, 55));
    ui::drawRoundedPanel(window, {avX, innerY, avSize, avSize}, 6.f,
                         sf::Color(35, 35, 44),
                         focused ? COL_ACCENT : COL_BORDER,
                         1.5f);

    std::string qmark = "?";
    float qScale = 0.35f;
    float qw = bodyFont.getTextWidth(qmark, qScale);
    bodyFont.drawText(window, qmark,
                      {cx - qw / 2.f, innerY + avSize / 2.f - 14.f},
                      qScale, sf::Color(80, 80, 95));

    innerY += avSize + 14.f;

    float arrowScale = 0.22f;
    std::string leftArrow  = "<";
    std::string rightArrow = ">";
    float lAW = bodyFont.getTextWidth(leftArrow,  arrowScale);
    float rAW = bodyFont.getTextWidth(rightArrow, arrowScale);

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mp(mi);
    float aY = innerY;
    bool lHov = sf::FloatRect(px + 15, aY - 8, 30, 28).contains(mp);
    bool rHov = sf::FloatRect(px + pW - 45, aY - 8, 30, 28).contains(mp);

    bodyFont.drawText(window, leftArrow,
                      {px + 24.f - lAW / 2.f, aY},
                      arrowScale * (lHov ? 1.25f : 1.0f),
                      lHov ? COL_ACCENT_L : COL_SUBTEXT);
    bodyFont.drawText(window, rightArrow,
                      {px + pW - 28.f - rAW / 2.f, aY},
                      arrowScale * (rHov ? 1.25f : 1.0f),
                      rHov ? COL_ACCENT_L : COL_SUBTEXT);

    for (int i = 0; i < 4; ++i) {
        sf::CircleShape dot(3.f);
        dot.setOrigin(3.f, 3.f);
        dot.setPosition(cx - 4.5f * 3 + i * 12.f, aY + 4.f);
        dot.setFillColor(i == avatarSlot ? COL_ACCENT : COL_BORDER);
        window.draw(dot);
    }
    innerY += 30.f;

    std::string nameLabel = "NAME";
    float nlScale = 0.14f;
    float nlw = bodyFont.getTextWidth(nameLabel, nlScale);
    bodyFont.drawText(window, nameLabel,
                      {cx - nlw / 2.f, innerY},
                      nlScale, COL_SUBTEXT);
    innerY += 20.f;

    float fW = pW * 0.82f;
    float fH = 38.f;
    float fX = cx - fW / 2.f;

    ui::drawRoundedPanel(window, {fX, innerY, fW, fH}, 5.f,
                         focused ? COL_ACTIVE : sf::Color(18, 18, 24),
                         focused ? COL_ACCENT : COL_BORDER,
                         1.0f);

    std::string displayText = field.text;
    if (focused && field.cursorVisible) displayText += "|";

    float tScale = 0.20f;
    while (bodyFont.getTextWidth(displayText, tScale) > fW - 20.f && tScale > 0.12f) {
        tScale -= 0.01f;
    }
    bodyFont.drawText(window, displayText,
                      {fX + 10.f, innerY + 10.f},
                      tScale, focused ? COL_TEXT : COL_SUBTEXT);
}

void PlayerSetupScreen::drawArrow(float cx, float cy, bool pointRight,
                                  float scale, bool hovered) {
    sf::ConvexShape arrow(3);
    float s = 10.f * scale;
    if (pointRight) {
        arrow.setPoint(0, {cx - s * 0.5f, cy - s});
        arrow.setPoint(1, {cx - s * 0.5f, cy + s});
        arrow.setPoint(2, {cx + s * 0.5f, cy});
    } else {
        arrow.setPoint(0, {cx + s * 0.5f, cy - s});
        arrow.setPoint(1, {cx + s * 0.5f, cy + s});
        arrow.setPoint(2, {cx - s * 0.5f, cy});
    }
    arrow.setFillColor(hovered ? COL_ACCENT_L : (scale > 1.05f ? COL_ACCENT : COL_SUBTEXT));
    window.draw(arrow);
}

void PlayerSetupScreen::drawRow(SetupSelectorRow& row, float cx, float y, bool focused) {
    float W      = (float)window.getSize().x;
    float cW     = W * 0.36f;
    float rowH   = 52.f;
    float rowX   = cx - cW / 2.f;

    ui::drawRoundedPanel(window, {rowX, y, cW, rowH}, 5.f,
                         focused ? COL_ACTIVE : sf::Color(22, 22, 28),
                         focused ? COL_ACCENT : COL_BORDER,
                         focused ? 1.5f : 1.0f);

    if (focused) {
        sf::RectangleShape bar({3.f, rowH});
        bar.setPosition(rowX, y);
        bar.setFillColor(COL_ACCENT);
        window.draw(bar);
    }

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mp(mi);
    float arrowCY = y + rowH / 2.f;

    // ← Left arrow
    float leftX = cx - 120.f;
    bool lHov = sf::FloatRect(leftX - 15, arrowCY - 15, 30, 30).contains(mp);
    drawArrow(leftX, arrowCY, false, row.leftScale, lHov);

    // → Right arrow
    float rightX = cx + 120.f;
    bool rHov = sf::FloatRect(rightX - 15, arrowCY - 15, 30, 30).contains(mp);
    drawArrow(rightX, arrowCY, true, row.rightScale, rHov);

    // Row label
    float lScale = 0.12f;
    bodyFont.drawText(window, row.label,
                      {rowX + 12.f, y + 4.f},
                      lScale, COL_SUBTEXT);

    if (row.label == "AI LEVEL" && !row.options.empty()) {
        int maxIdx = (int)row.options.size() - 1;
        float sliderX = cx - 82.f;
        float sliderY = y + 32.f;
        float sliderW = 164.f;
        float sliderH = 5.f;
        float ratio = maxIdx > 0 ? (float)row.currentIndex / (float)maxIdx : 0.f;

        ui::drawRoundedRect(window, {sliderX, sliderY, sliderW, sliderH}, 2.5f,
                            sf::Color(60, 60, 70));
        ui::drawRoundedRect(window, {sliderX, sliderY, sliderW * ratio, sliderH}, 2.5f,
                            focused ? COL_ACCENT_L : COL_ACCENT);

        sf::CircleShape knob(8.f);
        knob.setOrigin(8.f, 8.f);
        knob.setPosition(sliderX + sliderW * ratio, sliderY + sliderH / 2.f);
        knob.setFillColor(focused ? COL_ACCENT_L : COL_TEXT);
        knob.setOutlineThickness(1.f);
        knob.setOutlineColor(sf::Color(18, 18, 24));
        window.draw(knob);

        std::string levelText = "LEVEL " + row.current() + " / 20";
        float vScale = 0.16f;
        float vw = bodyFont.getTextWidth(levelText, vScale);
        bodyFont.drawText(window, levelText,
                          {cx - vw / 2.f, y + 15.f},
                          vScale, focused ? COL_TEXT : sf::Color(180, 180, 190));
        return;
    }

    // Current value
    float vScale = 0.18f;
    float vw = bodyFont.getTextWidth(row.current(), vScale);
    while (vw > cW * 0.42f && vScale > 0.13f) {
        vScale -= 0.01f;
        vw = bodyFont.getTextWidth(row.current(), vScale);
    }
    bodyFont.drawText(window, row.current(),
                      {cx - vw / 2.f, y + 18.f},
                      vScale, focused ? COL_TEXT : sf::Color(180, 180, 190));
}

void PlayerSetupScreen::drawButton(const std::string& label,
                                   sf::FloatRect rect, bool hovered) {
    ui::drawRoundedPanel(window, rect, 5.f,
                         hovered ? COL_ACCENT : sf::Color(30, 30, 38),
                         hovered ? COL_ACCENT_L : COL_BORDER,
                         1.5f);

    float scale = 0.22f;
    float tw = bodyFont.getTextWidth(label, scale);
    float ty = rect.top + (rect.height - scale * 50.f) / 2.f;
    bodyFont.drawText(window, label,
                      {rect.left + rect.width / 2.f - tw / 2.f, ty},
                      scale,
                      hovered ? sf::Color(10, 10, 12) : COL_TEXT);
}

void PlayerSetupScreen::draw() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    drawBackground();

    // ── Title ─────────────────────────────────────────────────────────────
    std::string title = aiMode ? "AI MATCH SETUP" : "LOCAL MATCH SETUP";
    float tScale = 0.42f;
    float tw = bodyFont.getTextWidth(title, tScale);
    bodyFont.drawText(window, title,
                      {W / 2.f - tw / 2.f, H * 0.06f},
                      tScale, sf::Color(210, 185, 60));

    sf::RectangleShape divider({280.f, 1.5f});
    divider.setPosition(W / 2.f - 140.f, H * 0.13f);
    divider.setFillColor(COL_ACCENT);
    window.draw(divider);

    // ── Columns ───────────────────────────────────────────────────────────
    float cW  = W * 0.36f;
    float pH  = H * 0.56f;
    float p1x = W * 0.04f;
    float cx  = W * 0.32f;
    float p2x = W * 0.72f;
    float py  = H * 0.20f;

    // Draw Player 1 (Left)
    drawPanel(p1x, py, "WHITE PLAYER", fields[0], avatarIndex[0]);

    // Draw Match Settings (Center)
    ui::drawRoundedPanel(window, {cx, py, cW, pH}, 7.f,
                         sf::Color(16, 16, 22, 240),
                         settingsFocused ? COL_ACCENT : COL_BORDER,
                         1.2f);

    std::string heading = aiMode ? "AI OPTIONS" : "MATCH OPTIONS";
    float hScale = 0.20f;
    float hw = bodyFont.getTextWidth(heading, hScale);
    bodyFont.drawText(window, heading, {cx + cW / 2.f - hw / 2.f, py + 16.f}, hScale, sf::Color(210, 185, 60));

    float rowH   = 52.f;
    float rowGap = 10.f;
    float startY = py + 48.f;
    for (int i = 0; i < (int)rows.size(); ++i) {
        drawRow(rows[i], cx + cW / 2.f, startY + i * (rowH + rowGap), settingsFocused && (i == focusedRow));
    }

    // Draw Player 2 (Right)
    drawPanel(p2x, py, aiMode ? "STOCKFISH AI" : "BLACK PLAYER", fields[1], avatarIndex[1]);

    // Hint text
    std::string hint = aiMode
        ? "TAB to switch panels  |  ENTER to start AI match  |  ARROWS to tune difficulty"
        : "TAB to switch panels  |  ENTER to start match  |  ARROWS to navigate & cycle";
    float hScaleHint = 0.14f;
    float hwHint = bodyFont.getTextWidth(hint, hScaleHint);
    bodyFont.drawText(window, hint,
                      {W / 2.f - hwHint / 2.f, H * 0.79f},
                      hScaleHint, COL_SUBTEXT);

    // Start Match button
    float btnW = 200.f, btnH = 48.f;
    float btnX = W / 2.f - btnW / 2.f;
    float btnY = H * 0.84f;
    bool  bothFilled = !fields[0].text.empty() && !fields[1].text.empty();
    sf::Vector2i mi = sf::Mouse::getPosition(window);
    bool  startHovB = sf::FloatRect(btnX, btnY, btnW, btnH).contains(sf::Vector2f(mi));
    drawButton("START MATCH", {btnX, btnY, btnW, btnH}, startHovB && bothFilled);

    // Back button
    bool backHovB = sf::FloatRect(40.f, 30.f, 100.f, 36.f).contains(sf::Vector2f(mi));
    drawButton("< BACK", {40.f, 30.f, 100.f, 36.f}, backHovB);
}
