#pragma once

#include <string>

#include "core/entity/rocket.hpp"
#include "core/physics/config.hpp"
#include "core/physics/rigidbody.hpp"
#include "core/physics/shape.hpp"
#include "core/physics/transform.hpp"

namespace YAML {

class Node;

}

namespace simulation {

namespace pathlib {

extern const std::string ROCKET_POLYGON_PATH;
extern const std::string MISSILE_POLYGON_PATH;
extern const std::string SIMULATION_CONFIG_PATH;

}

/** All values loaded from simulation-config.yaml. */
struct SimulationConfig {
    config::WorldConfig worldConfig;

    Transform rocketTransform;
    RigidBody rocketRigidBody;
    RocketEngine rocketEngine;

    Transform missileTransform;
    RigidBody missileRigidBody;

    explicit SimulationConfig(const std::string& path);

    PolygonLibrary makePolygon() const;

private:
    explicit SimulationConfig(const YAML::Node& root);
};

}
