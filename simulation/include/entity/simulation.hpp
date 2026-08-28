#pragma once
#include <cstddef>
#include <tuple>
#include <optional>

#include "rocket.hpp"
#include "transform.hpp"
#include "constants.hpp"
#include "vec2.hpp"

namespace simulation {


class Simulation {
private:
    Size spaceSize_ = {960, 540};
    Rocket rocket_ = {
        simulation::Transform({0., 0.}, {60, 60}),
        simulation::RigidBody(400, 400),
        {5.25 * 4000, 4500, 800}
    };
    constants::Physics constants_ = {9.81, 1.0f / 120};
    constants::Randomness randomness = {};

    const size_t timeoutStep_;
    size_t currentStep_ = 0;


public:
    Simulation(size_t timeoutStep, constants::Physics constant = {})
    : constants_(constant), timeoutStep_(timeoutStep)
    {
        reset();
    }

    Size spaceSize() const { return spaceSize_; }
    const Rocket& rocket() const { return rocket_; }

    void reset(std::optional<unsigned int> seed = std::nullopt) {
        rocket_.reset({(float) spaceSize_.width, (float) spaceSize_.height}, randomness.rocket, seed);
        currentStep_ = 0;
    }

    void update(const std::tuple<bool, bool, bool>& action) {
        currentStep_ += 1;
        rocket_.addExternalForce({0, -constants_.gravity * rocket_.rb().mass});
        rocket_.update(action, constants_.dt);
    }

    bool isTerminated() const {
        const Transform& tf = rocket_.transform();
        if (tf.right() < 0) return true;
        if (tf.left() > static_cast<float>(spaceSize_.width)) return true;
        if (tf.top() < 0) return true;
        if (tf.bottom() > static_cast<float>(spaceSize_.height)) return true;
        return false;
    }

    bool isTruncated() const {
        return timeoutStep_ <= currentStep_;
    }
};


}