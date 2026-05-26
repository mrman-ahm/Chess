#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <random>

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float opacity;
    float radius;
    float lifetime;
    float maxLifetime;
};

class ParticleSystem {
public:
    ParticleSystem(sf::Vector2u windowSize, int maxParticles = 60);
    void update(float dt);
    void draw(sf::RenderWindow& window);

private:
    void spawnParticle();
    sf::Vector2u windowSize;
    std::vector<Particle> particles;
    int maxParticles;
    float spawnTimer = 0.0f;
    float spawnRate = 0.3f;  // Seconds between spawns

    std::mt19937 rng;
    std::uniform_real_distribution<float> distX;
    std::uniform_real_distribution<float> distSpeed;
    std::uniform_real_distribution<float> distRadius;
    std::uniform_real_distribution<float> distLifetime;
};
