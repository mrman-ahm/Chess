#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include "BitmapFont.hpp"
#include "MenuButton.hpp"
#include "ParticleSystem.hpp"

enum class MenuAction { None, Play, PlayAI, Multiplayer, LoadGame, Settings, Credits, Exit };

class Menu {
public:
    Menu(sf::RenderWindow& window,
         BitmapFont& titleFont,
         BitmapFont& uiFont,
         std::map<char, sf::Texture>& textures);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw();

    MenuAction getAction() const { return pendingAction; }
    void clearAction() { pendingAction = MenuAction::None; }

    // Fade-in on scene entry
    void startFadeIn();
    bool isFadingIn() const { return fadeAlpha > 0.0f; }

private:
    void buildButtons();
    void drawBackground();
    void drawVignette();
    void drawTitle(float dt);
    void drawDivider();
    void drawShadowPiece();
    void drawCheckerboardAccent();

    sf::RenderWindow& window;
    BitmapFont& titleFont;
    BitmapFont& uiFont;
    std::map<char, sf::Texture>& textures;

    std::vector<MenuButton> buttons;
    std::vector<MenuAction> buttonActions;

    ParticleSystem particles;

    MenuAction pendingAction = MenuAction::None;
    int selectedIndex = 0;

    // Title bob animation
    float titleTimer = 0.0f;
    float titleY = 0.0f;

    // Fade-in overlay
    float fadeAlpha = 255.0f;

    // Hover line animation on divider
    float dividerWidth = 0.0f;

    // Background image
    sf::Texture bgTexture;
    sf::Sprite  bgSprite;
    bool bgLoaded = false;

    // Cached window size
    float W, H;
};
