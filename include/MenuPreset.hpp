#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <string>

struct MenuPresetConfig {
    const char* name;
    const char* titleFontFnt;
    const char* titleFontPng;
    const char* buttonFontFnt;
    const char* buttonFontPng;
    const char* backgroundPath;
    const char* decorativeQueenPath;
    bool showDecorativeQueen = true;
    bool showCheckerboard = false;
    sf::Color buttonBaseColor = sf::Color(215, 210, 190, 235);
    sf::Color buttonActiveColor = sf::Color(255, 225, 130, 255);
    sf::Color buttonGlowOuterColor = sf::Color(255, 210, 80, 45);
    sf::Color buttonGlowLineColor = sf::Color(255, 225, 120, 165);
    sf::Color titleColor = sf::Color(220, 195, 120, 255);
};

inline const std::array<MenuPresetConfig, 5>& getMenuPresets() {
    static const std::array<MenuPresetConfig, 5> presets = {{
        MenuPresetConfig{
            "Preset 1",
            "Sprites/Fonts/BlackStylishfont.fnt",
            "Sprites/Fonts/BlackStylishfont.png",
            "Sprites/Fonts/Bluefont.fnt",
            "Sprites/Fonts/Bluefont.png",
            "Sprites/Menu bg/background.jpg",
            "Sprites/Pieces/Set 9/W-Queen.png",
            true,
            false,
            sf::Color(215, 210, 190, 235),
            sf::Color(255, 225, 130, 255),
            sf::Color(255, 210, 80, 45),
            sf::Color(255, 225, 120, 165)
        },
        MenuPresetConfig{
            "Preset 2",
            "Sprites/Fonts/GrayStylishfont.fnt",
            "Sprites/Fonts/GrayStylishfont.png",
            "Sprites/Fonts/WhiteStylishfont.fnt",
            "Sprites/Fonts/WhiteStylishfont.png",
            "Sprites/Menu bg/menu2.jpg",
            "Sprites/Pieces/Set 9/W-Queen.png",
            false,
            false,
            sf::Color(215, 210, 190, 235),
            sf::Color(255, 225, 130, 255),
            sf::Color(255, 210, 80, 45),
            sf::Color(255, 225, 120, 165)
        },
        MenuPresetConfig{
            "Preset 3",
            "Sprites/Fonts/GrayStylishfont.fnt",
            "Sprites/Fonts/GrayStylishfont.png",
            "Sprites/Fonts/WhiteStylishfont.fnt",
            "Sprites/Fonts/WhiteStylishfont.png",
            "Sprites/Menu bg/menu3.jpg",
            "Sprites/Pieces/Set 9/W-Queen.png",
            false,
            false,
            sf::Color(215, 210, 190, 235),
            sf::Color(255, 225, 130, 255),
            sf::Color(255, 210, 80, 45),
            sf::Color(255, 225, 120, 165)
        },
        MenuPresetConfig{
            "Preset 4",
            "Sprites/Fonts/blue2.fnt",
            "Sprites/Fonts/blue2.png",
            "Sprites/Fonts/green.fnt",
            "Sprites/Fonts/green.png",
            "Sprites/Menu bg/menu4.jpg",
            "Sprites/Pieces/Set 9/W-Queen.png",
            false,
            false,
            sf::Color(215, 220, 225, 220),
            sf::Color(245, 250, 255, 238),
            sf::Color(245, 250, 255, 18),
            sf::Color(245, 250, 255, 88)
        },
        MenuPresetConfig{
            "Preset 5",
            "Sprites/Fonts/crystal1.fnt",
            "Sprites/Fonts/crystal1.png",
            "Sprites/Fonts/crystal2.fnt",
            "Sprites/Fonts/crystal2.png",
            "Sprites/Menu bg/menu5.jpg",
            "Sprites/Pieces/Set 9/W-Queen.png",
            false,
            false,
            sf::Color(218, 232, 240, 235),
            sf::Color(248, 253, 255, 255),
            sf::Color(210, 242, 255, 36),
            sf::Color(248, 253, 255, 150),
            sf::Color(255, 255, 255, 255)
        }
    }};
    return presets;
}

inline const MenuPresetConfig& findMenuPreset(const std::string& name) {
    const auto& presets = getMenuPresets();
    for (const auto& preset : presets) {
        if (name == preset.name) return preset;
    }
    return presets.back();
}
