#pragma once
#include <optional>

#include "core/model.hpp"
#include "core/math/vec2.hpp"
#include "core/physics/rigidbody.hpp"
#include "core/physics/transform.hpp"
#include "core/physics/config.hpp"
#include "core/physics/shape.hpp"

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

    RocketEngine engine_;

    const ShapeData& shape_;
public:

    Rocket(Transform, RigidBody, RocketEngine, ShapeData*);

    [[nodiscard]]
    const Transform& transform() const;

    [[nodiscard]]
    const RigidBody& rb() const;

    [[nodiscard]]
    const ShapeData& shape() const;

    void addExternalForce(Vec2 force);
    void addEngineForce(Input);
    void advance(Input input, float dt);
    void reset(const config::WorldConfig& config, std::optional<unsigned int> seed = std::nullopt);
};


}