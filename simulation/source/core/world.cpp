#include <optional>

#include "core/world.hpp"
#include "core/entity/missile.hpp"
#include "core/entity/rocket.hpp"
#include "core/model.hpp"
#include "core/physics/config.hpp"
#include "core/physics/geometry.hpp"
#include "core/physics/shape.hpp"

#include "asset/loader.hpp"

using namespace simulation;
using namespace simulation::config;

World::World(
    Rocket rocket,
    Missile missile,
    WorldConfig config
)
: worldConfig_(config), rocket_(rocket), missile_(missile), tick_(0U) {}

World::World(const SimulationConfig& simConfig, const PolygonLibrary& library)
: 
worldConfig_(simConfig.worldConfig),
rocket_(
    simConfig.rocketTransform,
    simConfig.rocketRigidBody,
    simConfig.rocketEngine,
    library.rocket,
    worldConfig_.random.rocket
),
missile_(
    simConfig.missileTransform,
    simConfig.missileRigidBody,
    library.missile,
    worldConfig_.random.missile
),
tick_(0U) {}

const Rocket& World::rocket() const noexcept {
    return rocket_;
}

const Missile& World::missile() const noexcept {
    return missile_;
}

const WorldConfig& World::worldConfig() const noexcept {
    return worldConfig_;
}

std::uint64_t World::tick() const noexcept {
    return tick_;
}

std::optional<WorldEvent> World::advance(Input input) {
    std::optional<WorldEvent> event = std::nullopt;
    tick_ += 1;

    // advance simulation
    rocket_.addExternalForce({0, -worldConfig_.physics.g * rocket_.rb().mass});
    rocket_.advance(input, worldConfig_.physics.dt);
    missile_.advance(worldConfig_.spaceSize, worldConfig_.physics.dt, rocket_.transform().center());

    // collision process
    if (isOutOfBound(rocket_.transform(), worldConfig_.spaceSize)) {
        event = WorldEvent::OutOfBound;
    }

    if (isCollide(rocket_.transform(), rocket_.polygon(), missile_.transform(), missile_.polygon())) {
        event = WorldEvent::MissileCollision;
    }

    if (isOutOfBound(missile_.transform(), worldConfig_.spaceSize)) {
        missile_.despawn();
        missile_.spawnSchedule();
    }

    return event;
}

void World::reset(std::optional<unsigned int> seed) {
    unsigned int count = 0;
    auto makeSeed = [&seed, &count] () -> std::optional<unsigned int> {
        if (seed.has_value()) {
            return (*seed) + (count++);
        }
        return std::nullopt;
    };

    tick_ = 0;
    rocket_.reset(worldConfig_.spaceSize, makeSeed());
    missile_.reset(makeSeed());
}
