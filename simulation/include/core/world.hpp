#pragma once
#include <optional>

#include "core/entity/missile.hpp"
#include "core/entity/rocket.hpp"
#include "core/model.hpp"
#include "core/physics/config.hpp"
#include "core/physics/shape.hpp"


namespace simulation {


class World {
private:
    Rocket rocket_;
    Missile missile_;
    config::WorldConfig worldConfig_;
    std::uint64_t tick_;

    const ShapeLibaray* shape_;

public:
    explicit World(Rocket, Missile, config::WorldConfig, const ShapeLibaray*);

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