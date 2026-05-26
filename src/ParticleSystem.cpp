#include "ParticleSystem.hpp"
#include <cmath>

ParticleSystem::ParticleSystem(sf::Vector2u winSize, int maxP)
    : windowSize(winSize), maxParticles(maxP),
      rng(std::random_device{}()),
      distX(0.0f, (float)winSize.x),
      distSpeed(4.0f, 14.0f),
      distRadius(1.0f, 3.5f),
      distLifetime(4.0f, 10.0f)
{
    // Pre-spawn some particles so the screen isn't empty at start
    for (int i = 0; i < maxParticles / 2; ++i) {
        spawnParticle();
        // Randomize their Y so they're spread out
        std::uniform_real_distribution<float> distY(0.0f, (float)winSize.y);
        particles.back().position.y = distY(rng);
    }
}

void ParticleSystem::spawnParticle() {
    Particle p;
    p.position = { distX(rng), (float)windowSize.y + 5.0f };
    float speed = distSpeed(rng);
    p.velocity = { (distX(rng) / windowSize.x - 0.5f) * 6.0f, -speed };
    p.radius = distRadius(rng);
    p.maxLifetime = distLifetime(rng);
    p.lifetime = p.maxLifetime;
    p.opacity = 0.0f;
    particles.push_back(p);
}

void ParticleSystem::update(float dt) {
    spawnTimer += dt;
    if (spawnTimer >= spawnRate && (int)particles.size() < maxParticles) {
        spawnParticle();
        spawnTimer = 0.0f;
    }

    for (auto it = particles.begin(); it != particles.end(); ) {
        it->lifetime -= dt;
        it->position += it->velocity * dt;

        // Gentle horizontal drift
        it->velocity.x += (distSpeed(rng) - 9.0f) * 0.3f * dt;

        // Life ratio
        float ratio = it->lifetime / it->maxLifetime;
        // Fade in quickly, hold, then fade out
        if (ratio > 0.85f)
            it->opacity = (1.0f - ratio) / 0.15f * 120.0f;
        else if (ratio < 0.2f)
            it->opacity = (ratio / 0.2f) * 120.0f;
        else
            it->opacity = 120.0f;

        if (it->lifetime <= 0.0f || it->position.y < -10.0f) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
}

void ParticleSystem::draw(sf::RenderWindow& window) {
    for (auto& p : particles) {
        sf::CircleShape dot(p.radius);
        dot.setOrigin(p.radius, p.radius);
        dot.setPosition(p.position);
        // Warm gold dust color
        dot.setFillColor(sf::Color(255, 220, 130, (sf::Uint8)p.opacity));
        window.draw(dot);
    }
}
