#pragma once
#include <tuple>
#include <random>
#include <optional>
#include <numbers>

#include "rigidbody.hpp"
#include "transform.hpp"
#include "vec2.hpp"

namespace simulation {
    

struct RocketEngine {
    const float mainForce;
    const float subForce;
    const float subTorque;
};


class Rocket {
private:
    Transform transform_;
    RigidBody rb_;

    RocketEngine engine;

public:
    Rocket(Transform transform, RigidBody body, RocketEngine engine)
    : transform_(transform), rb_(body), engine(engine) {}

    const Transform& transform() const { return transform_; }
    const RigidBody& rb() const { return rb_; }

    void addExternalForce(Vec2 force) { rb_.addForce(force); }
    void addEngineForce(const std::tuple<bool, bool, bool>& action) {
        auto [leftAction, mainAction, rightAction] = action;

        if (mainAction) { rb_.addForce(transform_.upUnit() * engine.mainForce); }
        if (leftAction) {
            rb_.addForce(transform_.upUnit() * engine.subForce);
            rb_.addTorque(-engine.subTorque);
        }
        if (rightAction) {
            rb_.addForce(transform_.upUnit() * engine.subForce);
            rb_.addTorque(engine.subTorque);
        }
    }

    void update(const std::tuple<bool, bool, bool>& action, float dt) {
        addEngineForce(action);
        rb_.update(dt);
        transform_.move(rb_.linearVel() * dt);
        transform_.rotate(rb_.angularVel() * dt);
    }

    void reset(Vec2 spaceSize, std::optional<unsigned int> seed = std::nullopt) {
        transform_.setCenter(spaceSize / 2);
        transform_.angle = 0;
        rb_.reset();

        if (seed.has_value()) {
            std::mt19937 rng(*seed);
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

            transform_.move({0.1f*spaceSize.x * dist(rng), 0.1f*spaceSize.y * dist(rng)});
            transform_.rotate(60.0f * static_cast<float>(std::numbers::pi)/180 * dist(rng));
        }
    }
};


}