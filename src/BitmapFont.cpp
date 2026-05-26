#include "BitmapFont.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

bool BitmapFont::loadFromFile(const std::string& fntPath, const std::string& pngPath) {
    charMap.clear();
    if (!texture.loadFromFile(pngPath)) {
        std::cerr << "Error: Could not load bitmap texture: " << pngPath << std::endl;
        return false;
    }
    texture.setSmooth(true);

    std::ifstream file(fntPath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open fnt file: " << fntPath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "char") {
            CharInfo info = {};
            int id = -1;
            std::string pair;
            while (ss >> pair) {
                size_t sep = pair.find('=');
                if (sep == std::string::npos) continue;
                std::string key = pair.substr(0, sep);
                std::string valStr = pair.substr(sep + 1);
                if (valStr.empty() || valStr[0] == '"') continue;
                try {
                    int val = std::stoi(valStr);
                    if      (key == "id")       id = val;
                    else if (key == "x")        info.x = val;
                    else if (key == "y")        info.y = val;
                    else if (key == "width")    info.width = val;
                    else if (key == "height")   info.height = val;
                    else if (key == "xoffset")  info.xoffset = val;
                    else if (key == "yoffset")  info.yoffset = val;
                    else if (key == "xadvance") info.xadvance = val;
                } catch (...) { continue; }
            }
            if (id != -1) charMap[id] = info;
        } else if (tag == "common") {
            std::string pair;
            while (ss >> pair) {
                if (pair.size() > 11 && pair.substr(0, 11) == "lineHeight=") {
                    try { lineHeight = std::stoi(pair.substr(11)); } catch(...) {}
                }
            }
        }
    }
    std::cout << "BitmapFont loaded: " << charMap.size() << " chars from " << fntPath << std::endl;
    return true;
}

void BitmapFont::drawText(sf::RenderWindow& window, const std::string& text,
                          sf::Vector2f position, float scale, sf::Color color,
                          const sf::Transform& transform) {
    sf::Vector2f curPos = position;
    sf::Sprite sprite(texture);
    sprite.setColor(color);

    sf::RenderStates states;
    states.transform = transform;

    for (unsigned char c : text) {
        auto it = charMap.find((int)c);
        if (it != charMap.end()) {
            const CharInfo& info = it->second;
            sprite.setTextureRect(sf::IntRect(info.x, info.y, info.width, info.height));
            sprite.setScale(scale, scale);
            sprite.setPosition(curPos.x + info.xoffset * scale,
                               curPos.y + info.yoffset * scale);
            window.draw(sprite, states);
            curPos.x += info.xadvance * scale;
        } else if (c == ' ') {
            curPos.x += 20.0f * scale;
        }
    }
}

float BitmapFont::getTextWidth(const std::string& text, float scale) const {
    float width = 0.0f;
    for (unsigned char c : text) {
        auto it = charMap.find((int)c);
        if (it != charMap.end())
            width += it->second.xadvance * scale;
        else if (c == ' ')
            width += 20.0f * scale;
    }
    return width;
}
