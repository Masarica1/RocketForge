#include <fstream>
#include <string>

#include <yaml-cpp/yaml.h>

#include "asset/json.hpp" // IWYU pragma: keep
#include "asset/loader.hpp"
#include "asset/yaml.hpp"

namespace simulation {

namespace pathlib {

const std::string ROCKET_POLYGON_PATH = "asset/rocket/polygon.json";
const std::string MISSILE_POLYGON_PATH = "asset/missile/polygon.json";
const std::string SIMULATION_CONFIG_PATH = "config/simulation-config.yaml";

}

SimulationConfig::SimulationConfig(const std::string& path)
    : SimulationConfig(YAML::LoadFile(path)) {}

SimulationConfig::SimulationConfig(const YAML::Node& root)
    : worldConfig(LoadWorldConfig(root)),
      rocketTransform(LoadTransform(root["entity"]["rocket"])),
      rocketRigidBody(LoadRigidBody(root["entity"]["rocket"])),
      rocketEngine {
          .mainForce = root["entity"]["rocket"]["engine"]["main_force"].as<float>(),
          .subForce = root["entity"]["rocket"]["engine"]["sub_force"].as<float>(),
          .subTorque = root["entity"]["rocket"]["engine"]["sub_torque"].as<float>()
      },
      missileTransform(LoadTransform(root["entity"]["missile"])),
      missileRigidBody(LoadRigidBody(root["entity"]["missile"])) {}

PolygonLibrary SimulationConfig::makePolygon() const {
    using namespace pathlib;

    PolygonLibrary library = {};

    nlohmann::json rocketJson;
    std::ifstream rocketJsonRaw(ROCKET_POLYGON_PATH);
    rocketJsonRaw >> rocketJson;
    library.rocket = rocketJson.get<ShapeData>().toWolrdPolygon(rocketTransform.size);

    nlohmann::json missileJson;
    std::ifstream missileJsonRaw(MISSILE_POLYGON_PATH);
    missileJsonRaw >> missileJson;
    library.missile = missileJson.get<ShapeData>().toWolrdPolygon(missileTransform.size);

    return library;
}

}
