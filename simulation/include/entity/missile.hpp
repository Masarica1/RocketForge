#pragma once
#include "optional"
#include "random"

#include "transform.hpp"
#include "rigidbody.hpp"
#include "constants.hpp"
#include "vec2.hpp"
#include <vector>

namespace simulation {

class Missile {
private:
    Transform transform_;
    RigidBody rb_;
    std::vector<Vec2> polygon_;

    float respawnDelaySeconds_ = 0;
    bool alive_ = false;

    std::mt19937 rng_{1};

    bool outOfBound(Size spaceSize) const;

public:
    Missile(Transform, RigidBody, Vec2 sourceSize);

    const Transform& transform() const;
    const RigidBody& rb() const;
    const std::vector<Vec2>& polygon() const;
    
    bool alive() const;
    
    void update(
        float dt,
        Size spaceSize,
        Vec2 targetCenter,
        const constants::Randomness::Missile& config
    );
    void reset(
        std::optional<unsigned int> seed,
        const constants::Randomness::Missile& config
    );
};

}
