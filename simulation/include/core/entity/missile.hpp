#pragma once
#include <optional>
#include <random>

#include "core/math/vec2.hpp"
#include "core/physics/rigidbody.hpp"
#include "core/physics/transform.hpp"
#include "core/physics/config.hpp"
#include "core/physics/shape.hpp"

namespace simulation {

class Missile {
private:
    Transform transform_;
    RigidBody rb_;

    const ShapeData& shape_;
    const config::Randomness::Missile config_;

    float respawnDelaySeconds_ = 0;
    bool alive_ = false;

    std::mt19937 rng_{std::random_device{}()};

public:
    Missile(Transform, RigidBody, const ShapeData&, const config::Randomness::Missile&);

    [[nodiscard]]
    const Transform& transform() const noexcept;

    [[nodiscard]]
    const RigidBody& rb() const noexcept;

    [[nodiscard]]
    const ShapeData& shape() const noexcept;
    
    [[nodiscard]]
    bool alive() const noexcept;

    [[nodiscard]]
    float respawnDelaySeconds() const noexcept;
    
    void advance(SizeInt size, float dt, Vec2 respawnTarget);
    void reset(std::optional<unsigned int> seed = std::nullopt);
    void spawnSchedule();
    void despawn();
};

}
