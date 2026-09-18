#pragma once

#include "core/physics/shape.hpp"
#include "core/world.hpp"

#include "asset/loader.hpp"


namespace simulation {

class Simulation {
private:
    PolygonLibrary polygonLibrary;
    World world;

    Simulation(const SimulationConfig& config)
    : polygonLibrary(config.makePolygon()), world(config, polygonLibrary) {}

public:
    Simulation(): Simulation(SimulationConfig(pathlib::SIMULATION_CONFIG_PATH)) {}
    
};

}