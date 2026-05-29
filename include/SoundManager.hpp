#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <vector>

struct MoveSoundInfo {
    bool capture = false;
    bool check = false;
    bool checkmate = false;
    bool rookCaptured = false;
};

class SoundManager {
public:
    void loadMoveSounds();
    void setFahhMode(bool enabled) { fahhMode = enabled; }
    bool isFahhMode() const { return fahhMode; }

    void update();
    void playPickup();
    void playMoveResolution(const MoveSoundInfo& info);
    void playMenuWelcome();
    void playMenuChange();
    void playMenuClick();

private:
    void loadSound(const std::string& key, const std::string& path);
    void play(const std::string& key);

    std::map<std::string, sf::SoundBuffer> buffers;
    std::vector<sf::Sound> activeSounds;
    bool fahhMode = false;
};
