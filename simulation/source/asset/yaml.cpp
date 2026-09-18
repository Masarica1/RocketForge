#include <cstddef>
#include <numbers>
#include <stdexcept>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "asset/yaml.hpp"
#include "core/math/vec2.hpp"

namespace simulation {

namespace {

Vec2 toVec2(const YAML::Node& node) {
    const auto values = node.as<std::vector<float>>();
    if (values.size() != 2) {
        throw YAML::RepresentationException(node.Mark(), "expected a sequence with two elements");
    }

    return {values[0], values[1]};
}

SizeInt toSizeInt(const YAML::Node& node) {
    const auto values = node.as<std::vector<std::size_t>>();
    if (values.size() != 2) {
        throw YAML::RepresentationException(node.Mark(), "expected a sequence with two elements");
    }

    return {values[0], values[1]};
}

float degreesToRadians(float degrees) {
    return degrees * static_cast<float>(std::numbers::pi) / 180.0f;
}

}

config::WorldConfig LoadWorldConfig(const YAML::Node& root) {
    config::WorldConfig parsed{};
    const YAML::Node configNode = root["config"];

    parsed.spaceSize = toSizeInt(configNode["space_size"]);

    const YAML::Node physicsNode = configNode["physics"];
    const float fps = physicsNode["fps"].as<float>();
    if (fps <= 0.0f) {
        throw std::invalid_argument("config.physics.fps must be greater than zero");
    }
    parsed.physics.dt = 1.0f / fps;
    parsed.physics.g = physicsNode["g"].as<float>();

    const YAML::Node rocketRandomness = configNode["rocket_randomness"];
    parsed.random.rocket.initPosRatio = toVec2(rocketRandomness["init_pos_ratio"]);
    parsed.random.rocket.initAngle =
        degreesToRadians(rocketRandomness["init_angle"].as<float>());
    parsed.random.rocket.initLinearVel = toVec2(rocketRandomness["init_linear_vel"]);
    parsed.random.rocket.initAngularVel =
        degreesToRadians(rocketRandomness["init_angular_vel"].as<float>());

    const YAML::Node missileRandomness = configNode["missile_randomness"];
    parsed.random.missile.targetPosError = toVec2(missileRandomness["target_pos_error"]);
    parsed.random.missile.minLinearVel = missileRandomness["min_linear_vel"].as<float>();
    parsed.random.missile.maxLinearVel = missileRandomness["max_linear_vel"].as<float>();
    parsed.random.missile.minRespawnTime = missileRandomness["min_respawn_time"].as<int>();
    parsed.random.missile.maxRespawnTime = missileRandomness["max_respawn_time"].as<int>();

    return parsed;
}

Transform LoadTransform(const YAML::Node& root) {
    return Transform {{0.0f, 0.0f}, toVec2(root["size"])};
}

RigidBody LoadRigidBody(const YAML::Node& root) {
    return RigidBody {
        root["mass"].as<float>(),
        root["moi"].as<float>()
    };
}

}
