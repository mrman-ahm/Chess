#include "Menu.hpp"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
Menu::Menu(sf::RenderWindow& win, BitmapFont& tf, BitmapFont& uf,
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
    : window(win), titleFont(tf), uiFont(uf), textures(tex),
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
    std::cout << "Menu::Menu body started" << std::endl;
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

void Menu::handleEvent(const sf::Event& event) {
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

    // Fade-in black overlay (drawn last so it covers everything)
    if (fadeAlpha > 0.0f) {
        sf::RectangleShape overlay(sf::Vector2f(W, H));
        overlay.setFillColor(sf::Color(0, 0, 0, (sf::Uint8)fadeAlpha));
        window.draw(overlay);
    }
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

    float scale = 1.4f;
    float tw = titleFont.getTextWidth("CHESS", scale);
    float tx = W * 0.28f - tw / 2.0f;  // centered over the button column
    float ty = H * 0.09f + bob;

    // Shadow pass (offset, dark)
    titleFont.drawText(window, "CHESS", sf::Vector2f(tx + 3.0f, ty + 4.0f), scale, sf::Color(0, 0, 0, 90));

    // Main title pass
    titleFont.drawText(window, "CHESS", sf::Vector2f(tx, ty), scale, titleColor);

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
