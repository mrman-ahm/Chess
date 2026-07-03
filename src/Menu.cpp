#include "Menu.hpp"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
Menu::Menu(sf::RenderWindow& win, BitmapFont& tf, BitmapFont& uf, BitmapFont& of,
           std::map<char, sf::Texture>& tex,
           const std::string& backgroundPath,
           const std::string& decorativeQueenPath,
           bool showQueen,
           bool showBoard,
           sf::Color baseColor,
           sf::Color activeColor,
           sf::Color glowOuterColor,
           sf::Color glowLineColor,
           sf::Color titleTint)
    : window(win), titleFont(tf), uiFont(uf), overlayFont(of), textures(tex),
      particles(win.getSize(), 55),
      showDecorativeQueen(showQueen),
      showCheckerboard(showBoard),
      buttonBaseColor(baseColor),
      buttonActiveColor(activeColor),
      buttonGlowOuterColor(glowOuterColor),
      buttonGlowLineColor(glowLineColor),
      titleColor(titleTint),
      W((float)win.getSize().x), H((float)win.getSize().y)
{
    buildButtons();
    titleY = H * 0.12f;
    dividerWidth = 0.0f;
    fadeAlpha = 255.0f;
    loadBackground(backgroundPath);
    loadDecorativeQueen(decorativeQueenPath);
}

void Menu::loadBackground(const std::string& path) {
    bgLoaded = false;
    bgSprite = sf::Sprite();
    if (!bgTexture.loadFromFile(path)) {
        std::cerr << "Warning: Could not load " << path << "\n";
    } else {
        bgSprite.setTexture(bgTexture);
        float sx = W / (float)bgTexture.getSize().x;
        float sy = H / (float)bgTexture.getSize().y;
        bgSprite.setScale(sx, sy);
        bgLoaded = true;
    }
}

void Menu::loadDecorativeQueen(const std::string& path) {
    decorativeQueenLoaded = false;
    if (!decorativeQueenTexture.loadFromFile(path)) {
        std::cerr << "Warning: Could not load " << path << "\n";
    } else {
        decorativeQueenLoaded = true;
    }
}

void Menu::setVisualConfig(const std::string& backgroundPath,
                           const std::string& decorativeQueenPath,
                           bool showQueen,
                           bool showBoard,
                           sf::Color baseColor,
                           sf::Color activeColor,
                           sf::Color glowOuterColor,
                           sf::Color glowLineColor,
                           sf::Color titleTint) {
    showDecorativeQueen = showQueen;
    showCheckerboard = showBoard;
    buttonBaseColor = baseColor;
    buttonActiveColor = activeColor;
    buttonGlowOuterColor = glowOuterColor;
    buttonGlowLineColor = glowLineColor;
    titleColor = titleTint;
    applyButtonColors();
    loadBackground(backgroundPath);
    loadDecorativeQueen(decorativeQueenPath);
}

void Menu::applyButtonColors() {
    for (auto& button : buttons) {
        button.setColors(buttonBaseColor, buttonActiveColor, buttonGlowOuterColor, buttonGlowLineColor);
    }
}

void Menu::buildButtons() {
    // Button labels and their corresponding actions
    std::vector<std::string>  labels  = { "LOCAL MATCH", "LOAD GAME", "PLAY VS AI", "MULTIPLAYER", "SETTINGS", "CREDITS", "EXIT" };
    std::vector<MenuAction>   actions = { MenuAction::Play, MenuAction::LoadGame, MenuAction::PlayAI,
                                          MenuAction::Multiplayer, MenuAction::Settings,
                                          MenuAction::Credits, MenuAction::Exit };

    buttons.clear();
    buttonActions.clear();

    // Left-third layout: buttons live at x = 28% of screen
    float bx = W * 0.28f;
    float startY = H * 0.335f;
    float stepY  = H * 0.082f;

    for (int i = 0; i < (int)labels.size(); ++i) {
        MenuButton btn;
        btn.setFont(&uiFont);
        btn.setText(labels[i]);
        btn.setBaseScale(0.36f);
        btn.setHoverScale(0.46f);
        btn.setColors(buttonBaseColor, buttonActiveColor, buttonGlowOuterColor, buttonGlowLineColor);
        float subtleDown = H * 0.014f;
        btn.setPosition(bx, startY + i * stepY + subtleDown);
        btn.setSelected(i == 0);
        buttons.push_back(btn);
        buttonActions.push_back(actions[i]);
    }

    selectedIndex = 0;
}

// ─────────────────────────────────────────────
//  Public Interface
// ─────────────────────────────────────────────
void Menu::startFadeIn() {
    fadeAlpha = 255.0f;
    welcomeSoundRequested = true;
}

bool Menu::consumeWelcomeSound() {
    bool requested = welcomeSoundRequested;
    welcomeSoundRequested = false;
    return requested;
}

bool Menu::consumeChangeSound() {
    bool requested = changeSoundRequested;
    changeSoundRequested = false;
    return requested;
}

bool Menu::consumeClickSound() {
    bool requested = clickSoundRequested;
    clickSoundRequested = false;
    return requested;
}

void Menu::showUnavailableNotice() {
    unavailableNoticeTimer = 2.2f;
}

void Menu::showCredits() {
    creditsOpen = true;
}

void Menu::refreshLayout() {
    W = static_cast<float>(window.getSize().x);
    H = static_cast<float>(window.getSize().y);
    particles = ParticleSystem(window.getSize(), 55);
    buildButtons();
    dividerWidth = std::min(dividerWidth, W * 0.38f);

    if (bgLoaded && bgTexture.getSize().x > 0 && bgTexture.getSize().y > 0) {
        bgSprite.setScale(W / static_cast<float>(bgTexture.getSize().x),
                          H / static_cast<float>(bgTexture.getSize().y));
    }
}

void Menu::handleEvent(const sf::Event& event) {
    if (creditsOpen) {
        if (event.type == sf::Event::KeyPressed &&
            (event.key.code == sf::Keyboard::Escape ||
             event.key.code == sf::Keyboard::Return ||
             event.key.code == sf::Keyboard::Space)) {
            creditsOpen = false;
            clickSoundRequested = true;
        } else if (event.type == sf::Event::MouseButtonPressed &&
                   event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mp = window.mapPixelToCoords(
                {event.mouseButton.x, event.mouseButton.y});
            sf::FloatRect backRect(W / 2.f - 75.f, H * 0.83f, 150.f, 42.f);
            if (backRect.contains(mp)) {
                creditsOpen = false;
                clickSoundRequested = true;
            }
        }
        return;
    }

    if (event.type == sf::Event::KeyPressed) {
        // Keyboard navigation
        if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::W) {
            buttons[selectedIndex].setSelected(false);
            selectedIndex = (selectedIndex + (int)buttons.size() - 1) % (int)buttons.size();
            buttons[selectedIndex].setSelected(true);
            changeSoundRequested = true;
        }
        if (event.key.code == sf::Keyboard::Down || event.key.code == sf::Keyboard::S) {
            buttons[selectedIndex].setSelected(false);
            selectedIndex = (selectedIndex + 1) % (int)buttons.size();
            buttons[selectedIndex].setSelected(true);
            changeSoundRequested = true;
        }
        if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Space) {
            pendingAction = buttonActions[selectedIndex];
            clickSoundRequested = true;
        }
        if (event.key.code == sf::Keyboard::Escape) {
            pendingAction = MenuAction::Exit;
            clickSoundRequested = true;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mp = window.mapPixelToCoords(
            { event.mouseButton.x, event.mouseButton.y });
        for (int i = 0; i < (int)buttons.size(); ++i) {
            if (buttons[i].wasClicked(mp, true)) {
                pendingAction = buttonActions[i];
                clickSoundRequested = true;
            }
        }
    }
}

void Menu::update(float dt) {
    titleTimer += dt;
    unavailableNoticeTimer = std::max(0.f, unavailableNoticeTimer - dt);

    // Grow the divider line on first frame
    float targetDiv = W * 0.38f;
    dividerWidth += (targetDiv - dividerWidth) * 6.0f * dt;
    if (dividerWidth > targetDiv) dividerWidth = targetDiv;

    // Fade-in overlay
    if (fadeAlpha > 0.0f) {
        fadeAlpha -= 280.0f * dt;
        if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
    }

    // Update particles
    particles.update(dt);

    // Update buttons — find mouse position
    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    // Sync keyboard selection highlight
    for (int i = 0; i < (int)buttons.size(); ++i) {
        buttons[i].setSelected(i == selectedIndex);
        buttons[i].update(dt, mp);

        // Let mouse override keyboard selection
        if (buttons[i].isHovered()) {
            if (selectedIndex != i) {
                buttons[selectedIndex].setSelected(false);
                selectedIndex = i;
                buttons[selectedIndex].setSelected(true);
                changeSoundRequested = true;
            }
        }
    }
}

// ─────────────────────────────────────────────
//  Draw
// ─────────────────────────────────────────────
void Menu::draw() {
    drawBackground();
    particles.draw(window);
    if (showDecorativeQueen) {
        drawShadowPiece();
    }
    if (showCheckerboard) {
        drawCheckerboardAccent();
    }
    drawTitle(0.0f);
    drawDivider();

    for (auto& btn : buttons)
        btn.draw(window);

    if (unavailableNoticeTimer > 0.f) {
        float alphaFactor = std::min(1.f, unavailableNoticeTimer * 3.f);
        sf::Uint8 alpha = static_cast<sf::Uint8>(220.f * alphaFactor);
        const float boxW = 390.f;
        const float boxH = 76.f;
        const float boxX = W / 2.f - boxW / 2.f;
        const float boxY = H * 0.80f;

        sf::RectangleShape shadow({boxW, boxH});
        shadow.setPosition(boxX + 5.f, boxY + 6.f);
        shadow.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(100.f * alphaFactor)));
        window.draw(shadow);

        sf::RectangleShape notice({boxW, boxH});
        notice.setPosition(boxX, boxY);
        notice.setFillColor(sf::Color(18, 18, 24, alpha));
        notice.setOutlineThickness(1.5f);
        notice.setOutlineColor(sf::Color(210, 210, 220, static_cast<sf::Uint8>(170.f * alphaFactor)));
        window.draw(notice);

        const std::string message = "NOT COMING SOON";
        const float messageScale = 0.28f;
        const float messageWidth = overlayFont.getTextWidth(message, messageScale);
        overlayFont.drawText(window, message,
                        {W / 2.f - messageWidth / 2.f, boxY + 23.f},
                        messageScale,
                        sf::Color(245, 245, 250, static_cast<sf::Uint8>(255.f * alphaFactor)));
    }

    if (creditsOpen) {
        drawCredits();
    }

    // Fade-in black overlay (drawn last so it covers everything)
    if (fadeAlpha > 0.0f) {
        sf::RectangleShape overlay(sf::Vector2f(W, H));
        overlay.setFillColor(sf::Color(0, 0, 0, (sf::Uint8)fadeAlpha));
        window.draw(overlay);
    }
}

void Menu::drawCredits() {
    sf::RectangleShape overlay({W, H});
    overlay.setFillColor(sf::Color(0, 0, 0, 205));
    window.draw(overlay);

    const float panelW = std::min(760.f, W * 0.72f);
    const float panelH = std::min(680.f, H * 0.76f);
    const float panelX = W / 2.f - panelW / 2.f;
    const float panelY = H / 2.f - panelH / 2.f;

    sf::RectangleShape shadow({panelW, panelH});
    shadow.setPosition(panelX + 7.f, panelY + 9.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 115));
    window.draw(shadow);

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition(panelX, panelY);
    panel.setFillColor(sf::Color(18, 18, 24, 248));
    panel.setOutlineThickness(1.5f);
    panel.setOutlineColor(sf::Color(175, 160, 110, 220));
    window.draw(panel);

    const std::string heading = "CREDITS";
    const float headingScale = 0.36f;
    const float headingWidth = overlayFont.getTextWidth(heading, headingScale);
    overlayFont.drawText(window, heading,
                    {W / 2.f - headingWidth / 2.f, panelY + 25.f},
                    headingScale, sf::Color(235, 220, 165));

    sf::RectangleShape divider({panelW - 80.f, 1.5f});
    divider.setPosition(panelX + 40.f, panelY + 78.f);
    divider.setFillColor(sf::Color(150, 135, 90, 180));
    window.draw(divider);

    static const std::vector<std::string> roles = {
        "CREATIVE DIRECTOR",
        "GAME DIRECTOR",
        "GAME DESIGNER",
        "PROJECT MANAGER",
        "LEAD DEVELOPER",
        "GAMEPLAY PROGRAMMER",
        "AI SYSTEMS PROGRAMMER",
        "UI / UX DESIGNER",
        "GRAPHIC DESIGNER",
        "TECHNICAL ARTIST",
        "ANIMATION DESIGNER",
        "SOUND DESIGNER",
        "QUALITY ASSURANCE",
        "BUILD & RELEASE"
    };

    const float rowsTop = panelY + 100.f;
    const float rowsBottom = panelY + panelH - 78.f;
    const float rowStep = (rowsBottom - rowsTop) / static_cast<float>(roles.size());
    const float roleScale = 0.15f;
    const float nameScale = 0.16f;
    const std::string name = "MUHAMMAD AHMAD";
    const float nameWidth = overlayFont.getTextWidth(name, nameScale);

    for (std::size_t i = 0; i < roles.size(); ++i) {
        const float y = rowsTop + static_cast<float>(i) * rowStep;
        overlayFont.drawText(window, roles[i], {panelX + 42.f, y}, roleScale,
                        sf::Color(150, 150, 165));
        overlayFont.drawText(window, name, {panelX + panelW - 42.f - nameWidth, y}, nameScale,
                        sf::Color(230, 230, 238));
    }

    sf::FloatRect backRect(W / 2.f - 75.f, H * 0.83f, 150.f, 42.f);
    sf::RectangleShape back({backRect.width, backRect.height});
    back.setPosition(backRect.left, backRect.top);
    back.setFillColor(sf::Color(34, 34, 42, 245));
    back.setOutlineThickness(1.5f);
    back.setOutlineColor(sf::Color(180, 165, 115, 220));
    window.draw(back);

    const std::string backText = "BACK";
    const float backScale = 0.20f;
    const float backWidth = overlayFont.getTextWidth(backText, backScale);
    overlayFont.drawText(window, backText,
                    {W / 2.f - backWidth / 2.f, backRect.top + 11.f},
                    backScale, sf::Color(235, 235, 240));
}

// ─────────────────────────────────────────────
//  Private Draw Helpers
// ─────────────────────────────────────────────
void Menu::drawBackground() {
    window.clear(sf::Color(20, 16, 5));
    if (bgLoaded) window.draw(bgSprite);
}

void Menu::drawVignette() {
    // Four semi-transparent corner rects simulating vignette
    float depth = W * 0.25f;
    sf::Color vig(0, 0, 0, 120);

    sf::RectangleShape left(sf::Vector2f(depth, H));
    left.setFillColor(vig);
    window.draw(left);

    sf::RectangleShape right(sf::Vector2f(depth, H));
    right.setPosition(W - depth, 0.0f);
    right.setFillColor(vig);
    window.draw(right);

    sf::RectangleShape top(sf::Vector2f(W, depth * 0.6f));
    top.setFillColor(vig);
    window.draw(top);

    sf::RectangleShape bottom(sf::Vector2f(W, depth * 0.6f));
    bottom.setPosition(0.0f, H - depth * 0.6f);
    bottom.setFillColor(vig);
    window.draw(bottom);
}

void Menu::drawTitle(float /*dt*/) {
    // Gentle bob using sin wave
    float bob = std::sin(titleTimer * 1.4f) * 4.0f;

    const std::string title = "SIXTY-FOUR";
    float scale = 1.4f;
    float tw = titleFont.getTextWidth(title, scale);
    const float maxTitleWidth = W * 0.42f;
    if (tw > maxTitleWidth && tw > 0.f) {
        scale *= maxTitleWidth / tw;
        tw = titleFont.getTextWidth(title, scale);
    }
    float tx = W * 0.28f - tw / 2.0f;  // centered over the button column
    float ty = H * 0.09f + bob;

    // Shadow pass (offset, dark)
    titleFont.drawText(window, title, sf::Vector2f(tx + 3.0f, ty + 4.0f), scale, sf::Color(0, 0, 0, 90));

    // Main title pass
    titleFont.drawText(window, title, sf::Vector2f(tx, ty), scale, titleColor);

    // Subtitle — placed below the main title
}

void Menu::drawDivider() {
    // Thin horizontal gold line beneath the title, above the buttons
    float lineY = H * 0.30f;
    float lineX = W * 0.28f - dividerWidth / 2.0f;

    // Outer glow
    sf::RectangleShape glow(sf::Vector2f(dividerWidth, 5.0f));
    glow.setPosition(lineX, lineY - 1.0f);
    glow.setFillColor(sf::Color(180, 145, 40, 30));
    window.draw(glow);

    // Sharp line
    sf::RectangleShape line(sf::Vector2f(dividerWidth, 1.5f));
    line.setPosition(lineX, lineY);
    line.setFillColor(sf::Color(180, 145, 40, 140));
    window.draw(line);
}

void Menu::drawShadowPiece() {
    if (!decorativeQueenLoaded || decorativeQueenTexture.getSize().y == 0) return;

    sf::Sprite queen(decorativeQueenTexture);
    queen.setColor(sf::Color(255, 255, 255, 18));
    float s = H / (float)decorativeQueenTexture.getSize().y * 0.95f;
    queen.setScale(s, s);
    float qw = decorativeQueenTexture.getSize().x * s;
    float qh = decorativeQueenTexture.getSize().y * s;
    queen.setPosition(W * 0.68f - qw / 2.0f, H - qh + H * 0.05f);
    window.draw(queen);
}

void Menu::drawCheckerboardAccent() {
    // A tiny 4×4 chessboard grid in the lower-right corner as a texture accent
    float tileSize = H * 0.045f;
    float startX = W * 0.82f;
    float startY = H * 0.72f;
    int gridSize = 4;

    for (int r = 0; r < gridSize; ++r) {
        for (int c = 0; c < gridSize; ++c) {
            bool light = (r + c) % 2 == 0;
            sf::RectangleShape tile(sf::Vector2f(tileSize, tileSize));
            tile.setPosition(startX + c * tileSize, startY + r * tileSize);
            // Muted, very low opacity chess colors
            if (light)
                tile.setFillColor(sf::Color(200, 175, 100, 30));
            else
                tile.setFillColor(sf::Color(60, 45, 25, 30));
            window.draw(tile);

            // Thin border on each tile
            sf::RectangleShape border(sf::Vector2f(tileSize, tileSize));
            border.setPosition(startX + c * tileSize, startY + r * tileSize);
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineThickness(0.5f);
            border.setOutlineColor(sf::Color(180, 145, 40, 25));
            window.draw(border);
        }
    }
}
