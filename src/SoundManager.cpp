#include "SoundManager.hpp"
#include <algorithm>
#include <iostream>

void SoundManager::loadMoveSounds() {
    buffers.clear();
    activeSounds.clear();

    loadSound("default.pickup", "Audio/Moves/Default/pickup.mp3");
    loadSound("default.dropdown", "Audio/Moves/Default/dropdown.mp3");
    loadSound("default.move", "Audio/Moves/Default/move.mp3");
    loadSound("default.capture", "Audio/Moves/Default/capture.mp3");
    loadSound("default.check", "Audio/Moves/Default/check.mp3");
    loadSound("default.checkmate", "Audio/Moves/Default/checkmate.mp3");

    loadSound("fahh.capture", "Audio/Moves/FAHH/kill.mp3");
    loadSound("fahh.check", "Audio/Moves/FAHH/check.mp3");
    loadSound("fahh.checkmate", "Audio/Moves/FAHH/checkmate.mp3");
    loadSound("fahh.rook", "Audio/Moves/FAHH/sacrifices-the-rook.mp3");

    loadSound("menu.welcome", "Audio/Moves/Menu/start game.mp3");
    loadSound("menu.change", "Audio/Moves/Menu/change.mp3");
    loadSound("menu.click", "Audio/Moves/Menu/click.mp3");
}

void SoundManager::loadSound(const std::string& key, const std::string& path) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(path)) {
        std::cerr << "Warning: sound not loaded: " << path << "\n";
        return;
    }
    buffers[key] = buffer;
}

void SoundManager::update() {
    activeSounds.erase(
        std::remove_if(activeSounds.begin(), activeSounds.end(), [](const sf::Sound& sound) {
            return sound.getStatus() == sf::Sound::Stopped;
        }),
        activeSounds.end());
}

void SoundManager::playPickup() {
    play("default.pickup");
}

void SoundManager::playMoveResolution(const MoveSoundInfo& info) {
    if (info.checkmate) {
        play(fahhMode ? "fahh.checkmate" : "default.checkmate");
    } else if (fahhMode && info.rookCaptured) {
        play("fahh.rook");
    } else if (info.check) {
        play(fahhMode ? "fahh.check" : "default.check");
    } else if (info.capture) {
        play(fahhMode ? "fahh.capture" : "default.capture");
    } else {
        play("default.dropdown");
    }
}

void SoundManager::playMenuWelcome() {
    play("menu.welcome");
}

void SoundManager::playMenuChange() {
    play("menu.change");
}

void SoundManager::playMenuClick() {
    play("menu.click");
}

void SoundManager::play(const std::string& key) {
    auto it = buffers.find(key);
    if (it == buffers.end()) return;

    activeSounds.emplace_back();
    activeSounds.back().setBuffer(it->second);
    activeSounds.back().play();
}
