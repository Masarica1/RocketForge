#include <random>

#include "core/entity/missile.hpp"
#include "core/math/vec2.hpp"
#include "core/physics/config.hpp"
#include "core/physics/rigidbody.hpp"
#include "core/physics/shape.hpp"
#include "core/physics/transform.hpp"

using namespace simulation;

Missile::Missile(
    Transform transform,
    RigidBody rb,
    const Polygon& polygon,
    const config::Randomness::Missile& config
): transform_(transform), rb_(rb), polygon_(polygon), config_(config) {}

const Transform& Missile::transform() const noexcept {
    return transform_;
}

const RigidBody& Missile::rb() const noexcept {
    return  rb_;
}

const Polygon& Missile::polygon() const noexcept {
    return polygon_;
}

bool Missile::alive() const noexcept {
    return alive_;
}

bool Missile::needToBeRespawn() const noexcept {
    return needToBeRespawn_;
}

float Missile::respawnDelaySeconds() const noexcept {
    return respawnDelaySeconds_;
}

void Missile::advance(SizeInt spaceSize, float dt, Vec2 respawnTarget) {
    if (alive_) {
        rb_.update(dt);
        transform_.move(rb_.linearVel() * dt);
        transform_.rotate(rb_.angularVel() * dt);
        return;
    }

    if (needToBeRespawn_) {
        respawnDelaySeconds_ -= dt;

        if (respawnDelaySeconds_ <= 0) {
            // respawn
            std::uniform_real_distribution<float> pDist(0.0f, 1.0f);
            std::uniform_real_distribution<float> rDist(-1.0f, 1.0f);

            if (pDist(rng_) < 0.5f) {
                transform_.setRight(0 - 0.5f * transform_.size.x);
            }
            else {
                transform_.setLeft(static_cast<float>(spaceSize.width) + 0.5f *transform_.size.x);
            }
            transform_.setCenterY(pDist(rng_) * static_cast<float>(spaceSize.height));

            float linearVel = config_.minLinearVel + (config_.maxLinearVel - config_.minLinearVel) * pDist(rng_);
            Vec2 direction = (respawnTarget - transform_.center()).normalized();
            rb_.setLinearVel(direction * linearVel);
            transform_.angle = std::atan2(direction.y, direction.x) - static_cast<float>(std::numbers::pi) / 2;
        }    
    }
}

void Missile::reset(std::optional<unsigned int> seed) {
    if (seed.has_value()) {
        rng_.seed(*seed);
    }

    despawn();
    spawnSchedule();
}

void Missile::despawn() {
    alive_ = false;
    needToBeRespawn_ = false;
}

void Missile::spawnSchedule() {
    alive_ = false;
    needToBeRespawn_ = true;

    std::uniform_real_distribution<float> dist((float) config_.minRespawnTime, (float) config_.maxRespawnTime);
    respawnDelaySeconds_ = dist(rng_);
}

