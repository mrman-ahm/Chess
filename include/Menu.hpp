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
         BitmapFont& overlayFont,
         std::map<char, sf::Texture>& textures,
         const std::string& backgroundPath,
         const std::string& decorativeQueenPath,
         bool showDecorativeQueen,
         bool showCheckerboard,
         sf::Color buttonBaseColor,
         sf::Color buttonActiveColor,
         sf::Color buttonGlowOuterColor,
         sf::Color buttonGlowLineColor,
         sf::Color titleColor);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw();
    void setVisualConfig(const std::string& backgroundPath,
                         const std::string& decorativeQueenPath,
                         bool showDecorativeQueen,
                         bool showCheckerboard,
                         sf::Color buttonBaseColor,
                         sf::Color buttonActiveColor,
                         sf::Color buttonGlowOuterColor,
                         sf::Color buttonGlowLineColor,
                         sf::Color titleColor);

    MenuAction getAction() const { return pendingAction; }
    void clearAction() { pendingAction = MenuAction::None; }
    bool consumeWelcomeSound();
    bool consumeChangeSound();
    bool consumeClickSound();
    void showUnavailableNotice();
    void showCredits();
    void refreshLayout();

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
    void drawCredits();
    void loadBackground(const std::string& path);
    void loadDecorativeQueen(const std::string& path);
    void applyButtonColors();

    sf::RenderWindow& window;
    BitmapFont& titleFont;
    BitmapFont& uiFont;
    BitmapFont& overlayFont;
    std::map<char, sf::Texture>& textures;

    std::vector<MenuButton> buttons;
    std::vector<MenuAction> buttonActions;

    ParticleSystem particles;

    MenuAction pendingAction = MenuAction::None;
    int selectedIndex = 0;
    bool welcomeSoundRequested = false;
    bool changeSoundRequested = false;
    bool clickSoundRequested = false;
    float unavailableNoticeTimer = 0.f;
    bool creditsOpen = false;

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
    sf::Texture decorativeQueenTexture;
    bool decorativeQueenLoaded = false;
    bool showDecorativeQueen = true;
    bool showCheckerboard = false;
    sf::Color buttonBaseColor = sf::Color(215, 210, 190, 235);
    sf::Color buttonActiveColor = sf::Color(255, 225, 130, 255);
    sf::Color buttonGlowOuterColor = sf::Color(255, 210, 80, 45);
    sf::Color buttonGlowLineColor = sf::Color(255, 225, 120, 165);
    sf::Color titleColor = sf::Color(220, 195, 120, 255);

    // Cached window size
    float W, H;
};
