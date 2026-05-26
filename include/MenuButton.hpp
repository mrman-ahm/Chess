#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "BitmapFont.hpp"

// Easing function types
enum class EaseType { Linear, EaseOut, EaseInOut };

class MenuButton {
public:
    MenuButton();

    void setText(const std::string& text);
    void setPosition(float x, float y);   // Sets the FIXED CENTER of the button
    void setBaseScale(float s);
    void setHoverScale(float s);
    void setFont(BitmapFont* font);

    void update(float dt, sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window);

    bool isHovered() const { return hovered; }
    bool wasClicked(sf::Vector2f mousePos, bool clicked);

    // For keyboard navigation
    void setSelected(bool selected);

private:
    std::string text;
    BitmapFont* font = nullptr;

    sf::Vector2f centerPos;   // Fixed anchor center
    float baseScale = 0.45f;
    float hoverScale = 0.65f;
    float currentScale = 0.45f;
    float targetScale = 0.45f;

    float opacity = 0.0f;     // For fade-in
    float targetOpacity = 255.0f;
    float activeAmount = 0.0f;

    bool hovered = false;
    bool selected = false;    // Keyboard selection

    // Glow line decoration (horizontal line under text)
    float glowWidth = 0.0f;
    float targetGlowWidth = 0.0f;

    static float lerp(float a, float b, float t);
    static float easeOut(float t);
};
