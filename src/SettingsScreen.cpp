#include "SettingsScreen.hpp"
#include "UIPrimitives.hpp"
#include <algorithm>
#include <cmath>

// ─── Palette (matches PlayerSetupScreen and game board) ────────────────────
static const sf::Color COL_BG_DARK  = sf::Color(14,  14,  18);
static const sf::Color COL_PANEL    = sf::Color(22,  22,  28);
static const sf::Color COL_BORDER   = sf::Color(70,  70,  82);
static const sf::Color COL_ACCENT   = sf::Color(173, 146, 29);
static const sf::Color COL_ACCENT_L = sf::Color(210, 185, 60);
static const sf::Color COL_TEXT     = sf::Color(220, 220, 232);
static const sf::Color COL_SUBTEXT  = sf::Color(140, 140, 158);
static const sf::Color COL_ROW_BG   = sf::Color(28,  28,  36);
static const sf::Color COL_ROW_FOC  = sf::Color(35,  35,  44);

SettingsScreen::SettingsScreen(sf::RenderWindow& win,
                                BitmapFont& title,
                                BitmapFont& body)
    : window(win), titleFont(title), bodyFont(body)
{
    buildRows();
}

void SettingsScreen::buildRows() {
    rows.clear();

    // ── Row 0 : Menu Background ────────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "MENU BACKGROUND";
        r.options = { "Default", "Dark", "Midnight", "Parchment", "Forest" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // ── Row 1 : Board Tile Theme ───────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "BOARD TILE THEME";
        r.options = { "Classic", "Wood", "Marble", "Ice", "Minimalist" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // ── Row 2 : Board Background ───────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "BOARD BACKGROUND";
        r.options = { "Texture", "Wood Table", "Dark Felt", "Marble Slab", "Plain" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // ── Row 3 : Board Perspective ──────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "BOARD PERSPECTIVE";
        r.options = { "White", "Black", "Auto" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    {
        SelectorRow r;
        r.label = "CURSOR STYLE";
        r.options = { "Chess", "System" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // ── Row 4 : Base Time ──────────────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "BASE TIME";
        r.options = { "1 Min", "3 Min", "5 Min", "10 Min", "15 Min", "30 Min", "60 Min" };
        r.currentIndex = 3; // Default to 10 Min
        rows.push_back(r);
    }
    // ── Row 5 : Time Increment ─────────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "INCREMENT (SEC)";
        r.options = { "0 Sec", "1 Sec", "2 Sec", "3 Sec", "5 Sec", "10 Sec", "15 Sec", "30 Sec" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // ── Row 6 : Time Delay ──────────────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "DELAY (SEC)";
        r.options = { "0 Sec", "1 Sec", "2 Sec", "3 Sec", "5 Sec", "10 Sec", "15 Sec", "30 Sec" };
        r.currentIndex = 0;
        rows.push_back(r);
    }

    rows.resize(5);
}

// ─── Events ───────────────────────────────────────────────────────────────
void SettingsScreen::handleEvent(const sf::Event& event) {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape)
            pendingAction = SettingsAction::Back;
        if (event.key.code == sf::Keyboard::Up)
            focusedRow = std::max(0, focusedRow - 1);
        if (event.key.code == sf::Keyboard::Down)
            focusedRow = std::min(std::min(4, (int)rows.size() - 1), focusedRow + 1);
        if (event.key.code == sf::Keyboard::Left) {
            rows[focusedRow].prev();
            rows[focusedRow].leftScale = 1.35f;   // trigger bulge
            rows[focusedRow].valuePulse = 1.0f;
        }
        if (event.key.code == sf::Keyboard::Right) {
            rows[focusedRow].next();
            rows[focusedRow].rightScale = 1.35f;
            rows[focusedRow].valuePulse = 1.0f;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {

        sf::Vector2f mp(event.mouseButton.x, event.mouseButton.y);
        float rowH    = 54.f;
        float rowGap  = 10.f;
        float panelW  = W * 0.52f;
        float panelX  = W / 2.f - panelW / 2.f;
        float startY  = H * 0.22f;

        int visibleRows = std::min(5, (int)rows.size());
        for (int i = 0; i < visibleRows; ++i) {
            float rowY = startY + i * (rowH + rowGap);
            float rowCX = panelX + panelW / 2.f;

            // Left arrow hit area
            sf::FloatRect leftRect(rowCX - 180.f, rowY + 7.f, 40.f, 40.f);
            // Right arrow hit area
            sf::FloatRect rightRect(rowCX + 140.f, rowY + 7.f, 40.f, 40.f);

            if (leftRect.contains(mp)) {
                focusedRow = i;
                rows[i].prev();
                rows[i].leftScale = 1.35f;
                rows[i].valuePulse = 1.0f;
            } else if (rightRect.contains(mp)) {
                focusedRow = i;
                rows[i].next();
                rows[i].rightScale = 1.35f;
                rows[i].valuePulse = 1.0f;
            } else if (sf::FloatRect(panelX, rowY, panelW, rowH).contains(mp)) {
                focusedRow = i;
            }
        }

        // Back button
        if (sf::FloatRect(40.f, 30.f, 100.f, 36.f).contains(mp))
            pendingAction = SettingsAction::Back;
    }
}

// ─── Update ───────────────────────────────────────────────────────────────
void SettingsScreen::update(float dt) {
    animTimer += dt;

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mp(mi);
    backHover += (sf::FloatRect(40.f, 30.f, 100.f, 36.f).contains(mp) ? dt : -dt) * 4.f;
    backHover  = std::clamp(backHover, 0.f, 1.f);

    // Decay bulge scale back to 1.0 smoothly
    for (int i = 0; i < (int)rows.size(); ++i) {
        auto& row = rows[i];
        row.leftScale  += (1.0f - row.leftScale)  * dt * 12.f;
        row.rightScale += (1.0f - row.rightScale) * dt * 12.f;
        row.focusAmount = ui::smoothToward(row.focusAmount, i == focusedRow ? 1.f : 0.f, dt, 10.f);
        row.valuePulse = ui::smoothToward(row.valuePulse, 0.f, dt, 7.f);
    }
}

// ─── Draw helpers ─────────────────────────────────────────────────────────
void SettingsScreen::drawBackground() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    sf::RectangleShape bg({W, H});
    bg.setFillColor(COL_BG_DARK);
    window.draw(bg);

    // Decorative large dim circle accent
    sf::CircleShape glow(H * 0.55f);
    glow.setOrigin(H * 0.55f, H * 0.55f);
    glow.setPosition(W / 2.f, H / 2.f);
    glow.setFillColor(sf::Color(173, 146, 29, 5));
    window.draw(glow);
}

void SettingsScreen::drawArrow(float cx, float cy, bool pointRight,
                                float scale, bool hovered) {
    // Arrow drawn as a simple triangle using sf::ConvexShape for clean rendering
    sf::ConvexShape arrow(3);
    float s = 14.f * scale;
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

void SettingsScreen::drawRow(SelectorRow& row, float cx, float y, bool focused) {
    (void)focused;
    float W      = (float)window.getSize().x;
    float panelW = W * 0.52f;
    float rowH   = 54.f;
    float rowX   = cx - panelW / 2.f;

    ui::drawRoundedRect(window, {rowX + 4, y + 5, panelW, rowH}, 6.f,
                        sf::Color(0, 0, 0, 50));
    float f = row.focusAmount;
    sf::Color rowFill = ui::mixColor(COL_ROW_BG, COL_ROW_FOC, f);
    sf::Color rowOutline = ui::mixColor(COL_BORDER, COL_ACCENT, f);
    ui::drawRoundedPanel(window, {rowX, y, panelW, rowH}, 6.f,
                         rowFill, rowOutline, 1.0f + 0.5f * f);

    // Left accent bar when focused
    if (f > 0.02f) {
        sf::RectangleShape bar({3.f, rowH});
        bar.setPosition(rowX, y);
        bar.setFillColor(sf::Color(COL_ACCENT.r, COL_ACCENT.g, COL_ACCENT.b, (sf::Uint8)(220 * f)));
        window.draw(bar);
    }

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mp(mi);
    float arrowCY = y + rowH / 2.f;

    // ← Left arrow
    float leftX = cx - 155.f;
    bool lHov = sf::FloatRect(leftX - 20, arrowCY - 20, 40, 40).contains(mp);
    drawArrow(leftX, arrowCY, false, row.leftScale, lHov);

    // → Right arrow
    float rightX = cx + 155.f;
    bool rHov = sf::FloatRect(rightX - 20, arrowCY - 20, 40, 40).contains(mp);
    drawArrow(rightX, arrowCY, true, row.rightScale, rHov);

    if (row.label == "MENU BACKGROUND" || row.label == "BOARD TILE THEME" || row.label == "BOARD BACKGROUND") {
        float pvW = 58.f, pvH = 34.f;
        float pvX = rowX + panelW - pvW - 18.f;
        float pvY = y + (rowH - pvH) / 2.f;
        ui::drawRoundedPanel(window, {pvX, pvY, pvW, pvH}, 4.f,
                             sf::Color(16, 16, 22, 235),
                             ui::mixColor(sf::Color(80, 80, 92), COL_ACCENT, f),
                             1.0f);

        sf::FloatRect inner(pvX + 6.f, pvY + 6.f, pvW - 12.f, pvH - 12.f);
        if (row.label == "BOARD TILE THEME") {
            float tile = inner.height / 2.f;
            for (int rr = 0; rr < 2; ++rr) {
                for (int cc = 0; cc < 3; ++cc) {
                    sf::RectangleShape sq({tile, tile});
                    sq.setPosition(inner.left + cc * tile, inner.top + rr * tile);
                    sq.setFillColor(((rr + cc) % 2 == 0) ? sf::Color(190, 175, 140, 180)
                                                         : sf::Color(80, 65, 45, 180));
                    window.draw(sq);
                }
            }
        } else {
            ui::drawRoundedRect(window, inner, 3.f,
                                row.label == "MENU BACKGROUND"
                                    ? sf::Color(38, 35, 45, 210)
                                    : sf::Color(52, 45, 36, 210));
            sf::RectangleShape stripe({inner.width, 2.f});
            stripe.setPosition(inner.left, inner.top + inner.height * 0.6f);
            stripe.setFillColor(sf::Color(210, 185, 80, 80));
            window.draw(stripe);
        }
    }

    // Row label (smaller, subtle)
    float lScale = 0.14f;
    bodyFont.drawText(window, row.label,
                      {rowX + 16.f, y + 5.f},
                      lScale, COL_SUBTEXT);

    // Current value (prominent, centered)
    float vScale = 0.22f;
    float vw = bodyFont.getTextWidth(row.current(), vScale);
    while (vw > panelW * 0.42f && vScale > 0.14f) {
        vScale -= 0.01f;
        vw = bodyFont.getTextWidth(row.current(), vScale);
    }
    float pulseScale = vScale + row.valuePulse * 0.018f;
    vw = bodyFont.getTextWidth(row.current(), pulseScale);
    bodyFont.drawText(window, row.current(),
                      {cx - vw / 2.f, y + 17.f},
                      pulseScale, ui::mixColor(sf::Color(190, 190, 200), COL_TEXT, f));
}

void SettingsScreen::drawButton(const std::string& label,
                                 sf::FloatRect rect, float hoverAmount) {
    ui::drawRoundedPanel(window, rect, 5.f,
                         ui::mixColor(sf::Color(28, 28, 36), COL_ACCENT, hoverAmount),
                         ui::mixColor(COL_BORDER, COL_ACCENT_L, hoverAmount),
                         1.5f);

    float scale = 0.20f;
    float tw = bodyFont.getTextWidth(label, scale);
    bodyFont.drawText(window, label,
                      {rect.left + rect.width / 2.f - tw / 2.f,
                       rect.top + (rect.height - scale * 50.f) / 2.f},
                      scale, ui::mixColor(COL_TEXT, sf::Color(10, 10, 12), hoverAmount));
}

// ─── Main draw ────────────────────────────────────────────────────────────
void SettingsScreen::draw() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    drawBackground();

    // ── Title ─────────────────────────────────────────────────────────────
    std::string title = "PERSONALIZATION";
    float tScale = 0.40f;
    float tw = bodyFont.getTextWidth(title, tScale);
    bodyFont.drawText(window, title,
                       {W / 2.f - tw / 2.f, H * 0.05f},
                       tScale, sf::Color(210, 185, 60));

    std::string sub = "SETTINGS";
    float subScale = 0.22f;
    float sw = bodyFont.getTextWidth(sub, subScale);
    bodyFont.drawText(window, sub,
                      {W / 2.f - sw / 2.f, H * 0.10f},
                      subScale, COL_SUBTEXT);

    // Gold divider
    sf::RectangleShape div({220.f, 1.5f});
    div.setPosition(W / 2.f - 110.f, H * 0.14f);
    div.setFillColor(COL_ACCENT);
    window.draw(div);

    // ── Section header: PERSONALIZATION ───────────────────────────────────
    std::string sec = "GAME OPTIONS & STYLES";
    float secScale = 0.16f;
    float secW = bodyFont.getTextWidth(sec, secScale);
    bodyFont.drawText(window, sec,
                      {W / 2.f - secW / 2.f, H * 0.17f},
                      secScale, COL_SUBTEXT);

    // ── Selector rows ─────────────────────────────────────────────────────
    float rowH   = 54.f;
    float rowGap = 10.f;
    float startY = H * 0.22f;
    float cx     = W / 2.f;

    int visibleRows = std::min(5, (int)rows.size());
    for (int i = 0; i < visibleRows; ++i) {
        drawRow(rows[i], cx, startY + i * (rowH + rowGap), i == focusedRow);
    }

    // ── Hint ──────────────────────────────────────────────────────────────
    std::string hint = "UP/DOWN to navigate  |  LEFT/RIGHT to change  |  CLICK arrows";
    float hScale = 0.13f;
    float hw = bodyFont.getTextWidth(hint, hScale);
    bodyFont.drawText(window, hint,
                      {W / 2.f - hw / 2.f, H * 0.88f},
                      hScale, COL_SUBTEXT);

    // ── Back button ───────────────────────────────────────────────────────
    drawButton("< BACK", {40.f, 30.f, 100.f, 36.f}, backHover);
    // Note: window.display() called by Game::render()
}

void SettingsScreen::setSelection(int rowIdx, const std::string& val) {
    if (rowIdx < 0 || rowIdx >= (int)rows.size()) return;
    auto& r = rows[rowIdx];
    for (int i = 0; i < (int)r.options.size(); ++i) {
        if (r.options[i] == val) {
            r.currentIndex = i;
            break;
        }
    }
}
