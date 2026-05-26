#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>

struct CharInfo {
    int x, y, width, height, xoffset, yoffset, xadvance;
};

class BitmapFont {
public:
    bool loadFromFile(const std::string& fntPath, const std::string& pngPath);

    // Draw with optional color tint (for shadows, fades, etc.) and transform
    void drawText(sf::RenderWindow& window, const std::string& text,
                  sf::Vector2f position, float scale = 1.0f,
                  sf::Color color = sf::Color::White,
                  const sf::Transform& transform = sf::Transform::Identity);

    float getTextWidth(const std::string& text, float scale = 1.0f) const;

private:
    sf::Texture texture;
    std::map<int, CharInfo> charMap;
    int lineHeight = 0;
};
