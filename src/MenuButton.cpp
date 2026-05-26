#include "MenuButton.hpp"
#include <cmath>

MenuButton::MenuButton() : currentScale(0.45f), targetScale(0.45f), opacity(0.0f), targetOpacity(255.0f) {}

void MenuButton::setText(const std::string& t) { text = t; }
void MenuButton::setPosition(float x, float y) { centerPos = { x, y }; }
void MenuButton::setBaseScale(float s) { baseScale = s; currentScale = s; }
void MenuButton::setHoverScale(float s) { hoverScale = s; }
void MenuButton::setFont(BitmapFont* f) { font = f; }
void MenuButton::setSelected(bool s) { selected = s; }

float MenuButton::lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float MenuButton::easeOut(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

void MenuButton::update(float dt, sf::Vector2f mousePos) {
    if (!font) return;

    // Determine if mouse is near this button's center
    float approxHalfW = font->getTextWidth(text, hoverScale) * 0.6f;
    float approxHalfH = 30.0f;
    bool mouseOver = (std::abs(mousePos.x - centerPos.x) < approxHalfW + 20.0f &&
                      std::abs(mousePos.y - centerPos.y) < approxHalfH + 15.0f);

    hovered = mouseOver;
    bool active = hovered || selected;

    targetScale = active ? hoverScale : baseScale;
    targetGlowWidth = active ? font->getTextWidth(text, hoverScale) * 0.85f : 0.0f;
    targetOpacity = 255.0f;

    // Smooth lerp — eased
    float speed = 12.0f * dt;
    currentScale = lerp(currentScale, targetScale, std::min(speed, 1.0f));
    activeAmount = lerp(activeAmount, active ? 1.0f : 0.0f, std::min(10.0f * dt, 1.0f));
    glowWidth = lerp(glowWidth, targetGlowWidth, std::min(speed * 0.8f, 1.0f));
    opacity = lerp(opacity, targetOpacity, std::min(8.0f * dt, 1.0f));
}

void MenuButton::draw(sf::RenderWindow& window) {
    if (!font) return;

    float w = font->getTextWidth(text, currentScale);

    // Center-anchored position
    float drawX = centerPos.x - w / 2.0f;
    float fontHeight = 60.0f * currentScale; // approx from fnt lineHeight=113 at scale 0.5
    float drawY = centerPos.y - fontHeight / 2.0f;

    // Draw glow underline — positioned BELOW the text.
    // Font lineHeight = 113 units. At currentScale, the text baseline sits at
    // drawY + lineHeight * scale. We draw 6px below that baseline.
    if (glowWidth > 2.0f) {
        float lineHeight = 113.0f;
        float textBottom = (centerPos.y - fontHeight / 2.0f) + lineHeight * currentScale + 6.0f;
        float ratio = glowWidth / font->getTextWidth(text, hoverScale);

        // Outer soft glow bar
        sf::RectangleShape glowBar(sf::Vector2f(glowWidth + 18.0f, 4.0f));
        glowBar.setOrigin((glowWidth + 18.0f) / 2.0f, 2.0f);
        glowBar.setPosition(centerPos.x, textBottom);
        glowBar.setFillColor(sf::Color(255, 210, 80, (sf::Uint8)(45 * ratio)));
        window.draw(glowBar);

        // Inner sharp glow line
        sf::RectangleShape line(sf::Vector2f(glowWidth, 1.4f));
        line.setOrigin(glowWidth / 2.0f, 0.7f);
        line.setPosition(centerPos.x, textBottom);
        line.setFillColor(sf::Color(255, 225, 120, (sf::Uint8)(165 * ratio)));
        window.draw(line);
    }

    // Draw the text itself with a softened hover transition.
    sf::Color base(215, 210, 190, 235);
    sf::Color active(255, 225, 130, 255);
    auto mix = [this](sf::Uint8 a, sf::Uint8 b) {
        return (sf::Uint8)(a + (b - a) * activeAmount);
    };
    sf::Color textColor(mix(base.r, active.r), mix(base.g, active.g),
                        mix(base.b, active.b), mix(base.a, active.a));
    font->drawText(window, text, sf::Vector2f(drawX, drawY), currentScale, textColor);
}

bool MenuButton::wasClicked(sf::Vector2f mousePos, bool clicked) {
    (void)mousePos;
    return clicked && hovered;
}
