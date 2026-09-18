#pragma once
#include <optional>

#include "core/entity/missile.hpp"
#include "core/entity/rocket.hpp"
#include "core/model.hpp"
#include "core/physics/config.hpp"

#include "asset/loader.hpp"
#include "core/physics/shape.hpp"


namespace simulation {


class World {
private:
    config::WorldConfig worldConfig_;
    Rocket rocket_;
    Missile missile_;

    std::uint64_t tick_;

public:
    World(Rocket, Missile, config::WorldConfig);
    World(const SimulationConfig&, const PolygonLibrary&);

    [[nodiscard]]
    const Rocket& rocket() const noexcept;

    [[nodiscard]]
    const Missile& missile() const noexcept;

    [[nodiscard]]
    const config::WorldConfig& worldConfig() const noexcept;

    [[nodiscard]]
    std::uint64_t tick() const noexcept;

    std::optional<WorldEvent> advance(Input);
    void reset(std::optional<unsigned int> seed = std::nullopt);
};

}