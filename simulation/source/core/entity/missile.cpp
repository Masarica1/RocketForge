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
    const ShapeData& shape,
    const config::Randomness::Missile& config
): transform_(transform), rb_(rb), shape_(shape), config_(config) {}

const Transform& Missile::transform() const noexcept {
    return transform_;
}

const RigidBody& Missile::rb() const noexcept {
    return  rb_;
}

const ShapeData& Missile::shape() const noexcept {
    return shape_;
}

bool Missile::alive() const noexcept {
    return alive_;
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

        

    }
}