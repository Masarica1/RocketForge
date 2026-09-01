#include <cmath>
#include <numbers>
#include <optional>
#include <random>
#include <vector>

#include "missile.hpp"
#include "rigidbody.hpp"
#include "transform.hpp"
#include "vec2.hpp"
#include "json.hpp"

namespace simulation {

bool Missile::outOfBound(Size spaceSize) const {
    if (
        transform_.right() < - 1.5f * transform_.size.x
        || transform_.left() > static_cast<float>(spaceSize.width) + 1.5f * transform_.size.x
        || transform_.top() < - 1.5f * transform_.size.y
        || transform_.bottom() > static_cast<float>(spaceSize.height) + 1.5f * transform_.size.y
    ) {
        return true;
    }
    return false;
}

Missile::Missile(Transform transform, RigidBody rb, Vec2 sourceSize)
: transform_(transform), rb_(rb)
{
    polygon_ = getPolygon("asset/missile/polygon.json", sourceSize, transform.size);
}

const Transform& Missile::transform() const {
    return transform_;
}

const RigidBody& Missile::rb() const {
    return rb_;
}

const std::vector<Vec2>& Missile::polygon() const {
    return polygon_;
}

bool Missile::alive() const {
    return alive_;
}

void Missile::update(
    float dt,
    Size spaceSize,
    Vec2 targetCenter,
    const constants::Randomness::Missile& config
) {
    if (alive_) {
        rb_.update(dt);
        transform_.move(rb_.linearVel() * dt);
        transform_.rotate(rb_.angularVel() * dt);

        if (outOfBound(spaceSize)) reset(std::nullopt, config);
    }
    else {
        if (respawnDelaySeconds_ > 0) {
            respawnDelaySeconds_ -= dt;
            return;
        }
        alive_ = true;
        rb_.reset();

        // respawn series
        std::uniform_real_distribution<float> dist1(0.0f, 1.0f);
        std::uniform_real_distribution<float> dist2(-1.0f, 1.0f);
    
        if (dist1(rng_) > 0.5f) {
            transform_.setRight(0.0f - transform_.size.x);
        }
        else {
            transform_.setLeft(static_cast<float>(spaceSize.width) + transform_.size.x);
        }
        transform_.setCenterY(dist1(rng_) * static_cast<float>(spaceSize.height));

        float linearVel = config.minLinearVel + (config.maxLinearVel - config.minLinearVel) * dist1(rng_);
        Vec2 targetVec = targetCenter - transform_.center() + (config.targetPosError * Vec2(dist2(rng_), dist2(rng_)));
        rb_.setLinearVel(targetVec.normalized() * linearVel);
        transform_.angle = std::atan2(targetVec.y, targetVec.x) - static_cast<float>(std::numbers::pi) / 2;
    }
}

void Missile::reset(
    std::optional<unsigned int> seed,
    const constants::Randomness::Missile& config
) {
    if (seed.has_value()) { rng_.seed(*seed); }

    alive_ = false;
    std::uniform_real_distribution<float> dist((float) config.minRespawnTime, (float) config.maxRespawnTime);
    respawnDelaySeconds_ = dist(rng_);
}

}