#include <random>

#include "core/entity/rocket.hpp"
#include "core/math/vec2.hpp"
#include "core/physics/rigidbody.hpp"
#include "core/physics/shape.hpp"
#include "core/physics/transform.hpp"

using namespace simulation;

Rocket::Rocket(
    Transform transform,
    RigidBody rb,
    RocketEngine engine,
    const ShapeData& shape
)
: transform_(transform), rb_(rb), engine_(engine), shape_(shape) {}

const Transform& Rocket::transform() const {
    return transform_;
}

const RigidBody& Rocket::rb() const {
    return rb_;
}

const ShapeData& Rocket::shape() const {
    return shape_;
}

void Rocket::addExternalForce(Vec2 force) {
    rb_.addForce(force);
}

void Rocket::addEngineForce(Input input) {
    if (input.main) {
        rb_.addForce(transform_.upUnit() * engine_.mainForce);
    }
    if (input.left) {
        rb_.addForce(transform_.upUnit() * engine_.subForce);
        rb_.addTorque(-engine_.subTorque);
    }
    if (input.right) {
        rb_.addForce(transform_.upUnit() * engine_.subForce);
        rb_.addTorque(engine_.subTorque);
    }
}

void Rocket::advance(Input input, float dt) {
    addEngineForce(input);
    rb_.update(dt);
    transform_.move(rb_.linearVel() * dt);
    transform_.rotate(rb_.angularVel() * dt);
}

void Rocket::reset(
    SizeInt spaceSize,
    const config::Randomness::Rocket& config,
    std::optional<unsigned int> seed
) {
    if (seed.has_value()) {rng_.seed(*seed);}

    // reset initial state
    transform_.setCenter(spaceSize.toVec() / 2);
    transform_.angle = 0.0f;
    rb_.reset();

    // randomize initial state
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    transform_.move(config.initPosRatio * spaceSize.toVec() * Vec2(dist(rng_), dist(rng_)));
    transform_.rotate(config.initAngle * dist(rng_)); 
    rb_.setLinearVel(config.initLinearVel * Vec2(dist(rng_), dist(rng_)));
    rb_.setAngularVel(config.initAngularVel * dist(rng_));  
}