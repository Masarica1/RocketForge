#pragma once

#include <numbers>

#include "vec2.hpp"

namespace simulation::constants {

struct Physics {
    float gravity = 9.81;
    float dt = 1.0 / 120;
};

struct Randomness {
    struct Rocket {
        Vec2 initPosRatio = {0.20, 0.20};
        float initAngle = 90 * static_cast<float>(std::numbers::pi)/180;
        Vec2 initLinearVel = {50, 25};
        float initAngularVel = 45 * static_cast<float>(std::numbers::pi)/180;
    };

    struct Missile {
        Vec2 targetPosError = {75, 75};
        float minLinearVel = 50;
        float maxLinearVel = 150;
        int minRespawnTime = 1;
        int maxRespawnTime = 4;
    };

    Rocket rocket = {};
    Missile missile = {};
};




}