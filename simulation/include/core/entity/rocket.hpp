#pragma once
#include <optional>
#include <random>

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

    const Polygon& polygon_;
    const config::Randomness::Rocket& config_;

    std::mt19937 rng_{std::random_device{}()};
public:

    Rocket(Transform, RigidBody, RocketEngine, const Polygon&, const config::Randomness::Rocket&);

    [[nodiscard]]
    const Transform& transform() const;

    [[nodiscard]]
    const RigidBody& rb() const;

    [[nodiscard]]
    const Polygon& polygon() const;

    void addExternalForce(Vec2 force);
    void addEngineForce(Input);
    void advance(Input input, float dt);

    /** reset the state of rocket
    * @param spaceSize size of world space.
    * @param config config data of rocket randomness
    * @param seed seed data for rng. use last seeded rng contiously when nullopt 
    */
    void reset(
        SizeInt spaceSize,
        std::optional<unsigned int> seed = std::nullopt
    );
};


}