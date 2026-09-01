#pragma once
#include <cstddef>
#include <tuple>
#include <optional>

#include "rigidbody.hpp"
#include "rocket.hpp"
#include "missile.hpp"
#include "transform.hpp"
#include "constants.hpp"
#include "collide.hpp"
#include "vec2.hpp"

namespace simulation {


class Simulation {
private:
    Size spaceSize_ = {960, 540};
    Rocket rocket_ = {
        Transform({0., 0.}, {60, 60}),
        RigidBody(400, 400),
        {5.25 * 4000, 4500, 800},
        {512, 512}
    };
    Missile missile_ = {
        Transform({0.0f, 0.0f}, {60, 60}),
        RigidBody(200, 200),
        {512, 512}
    };
    constants::Physics constants_ = {9.81, 1.0f / 120};
    constants::Randomness randomness_ = {};

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
    const Missile& missile() const { return missile_; }

    void reset(std::optional<unsigned int> seed = std::nullopt) {
        rocket_.reset({(float) spaceSize_.width, (float) spaceSize_.height}, randomness_.rocket, seed);
        missile_.reset(seed.has_value() ? std::optional<unsigned int>((*seed) + 1) : std::nullopt, randomness_.missile);
        currentStep_ = 0;
    }

    void update(const std::tuple<bool, bool, bool>& action) {
        currentStep_ += 1;
        rocket_.addExternalForce({0, -constants_.gravity * rocket_.rb().mass});
        rocket_.update(action, constants_.dt);
        missile_.update(constants_.dt, spaceSize_, rocket_.transform().center(), randomness_.missile);
    }

    bool isTerminated() const {
        const Transform& tf = rocket_.transform();
        if (
            tf.right() < 0
            || tf.left() > static_cast<float>(spaceSize_.width)
            || tf.top() < 0
            || tf.bottom() > static_cast<float>(spaceSize_.height)
        ) {
            return true;
        }

        if (
            missile_.alive()
            && isCollide(
                rocket_.transform(), rocket_.polygon(),
                missile_.transform(), missile_.polygon()
            )
        ) {
            return true;
        }


        return false;
    }

    bool isTruncated() const {
        return timeoutStep_ <= currentStep_;
    }
};


}