#pragma once

#include "core/physics/config.hpp"
#include "core/physics/rigidbody.hpp"
#include "core/physics/transform.hpp"

namespace YAML {

class Node;

}

namespace simulation {

config::WorldConfig LoadWorldConfig(const YAML::Node& root);

Transform LoadTransform(const YAML::Node& root);

RigidBody LoadRigidBody(const YAML::Node& root);

}
