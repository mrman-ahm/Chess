#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>

namespace ui {

inline float smoothToward(float current, float target, float dt, float speed) {
    float t = std::min(1.f, std::max(0.f, dt * speed));
    return current + (target - current) * t;
}

inline sf::Color mixColor(sf::Color a, sf::Color b, float t) {
    t = std::min(1.f, std::max(0.f, t));
    auto mix = [t](sf::Uint8 x, sf::Uint8 y) {
        return (sf::Uint8)(x + (y - x) * t);
    };
    return sf::Color(mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a));
}

inline float roundedRadius(sf::FloatRect rect, float radius) {
    return std::max(0.f, std::min(radius, std::min(rect.width, rect.height) * 0.5f));
}

inline void drawRoundedRect(sf::RenderTarget& target,
                            sf::FloatRect rect,
                            float radius,
                            sf::Color fill,
                            const sf::RenderStates& states = sf::RenderStates::Default) {
    radius = roundedRadius(rect, radius);
    if (rect.width <= 0.f || rect.height <= 0.f) return;

    if (radius <= 0.5f) {
        sf::RectangleShape box({rect.width, rect.height});
        box.setPosition(rect.left, rect.top);
        box.setFillColor(fill);
        target.draw(box, states);
        return;
    }

    sf::RectangleShape center({std::max(0.f, rect.width - 2.f * radius),
                               std::max(0.f, rect.height - 2.f * radius)});
    center.setPosition(rect.left + radius, rect.top + radius);
    center.setFillColor(fill);
    target.draw(center, states);

    sf::RectangleShape top({std::max(0.f, rect.width - 2.f * radius), radius});
    top.setPosition(rect.left + radius, rect.top);
    top.setFillColor(fill);
    target.draw(top, states);

    sf::RectangleShape bottom(top);
    bottom.setPosition(rect.left + radius, rect.top + rect.height - radius);
    target.draw(bottom, states);

    sf::RectangleShape left({radius, std::max(0.f, rect.height - 2.f * radius)});
    left.setPosition(rect.left, rect.top + radius);
    left.setFillColor(fill);
    target.draw(left, states);

    sf::RectangleShape right(left);
    right.setPosition(rect.left + rect.width - radius, rect.top + radius);
    target.draw(right, states);

    sf::CircleShape corner(radius);
    corner.setFillColor(fill);
    corner.setPosition(rect.left, rect.top);
    target.draw(corner, states);
    corner.setPosition(rect.left + rect.width - 2.f * radius, rect.top);
    target.draw(corner, states);
    corner.setPosition(rect.left, rect.top + rect.height - 2.f * radius);
    target.draw(corner, states);
    corner.setPosition(rect.left + rect.width - 2.f * radius, rect.top + rect.height - 2.f * radius);
    target.draw(corner, states);
}

inline void drawRoundedPanel(sf::RenderTarget& target,
                             sf::FloatRect rect,
                             float radius,
                             sf::Color fill,
                             sf::Color outline,
                             float thickness = 1.f,
                             const sf::RenderStates& states = sf::RenderStates::Default) {
    if (thickness <= 0.f) {
        drawRoundedRect(target, rect, radius, fill, states);
        return;
    }

    drawRoundedRect(target, rect, radius, outline, states);
    sf::FloatRect inner(rect.left + thickness, rect.top + thickness,
                        rect.width - 2.f * thickness, rect.height - 2.f * thickness);
    drawRoundedRect(target, inner, std::max(0.f, radius - thickness), fill, states);
}

} // namespace ui
