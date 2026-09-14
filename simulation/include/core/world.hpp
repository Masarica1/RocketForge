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

    const ShapeLibaray* shape;

public:
    explicit World(Rocket, Missile, config::WorldConfig);

    [[nodiscard]]
    const Rocket& rocket() const noexcept;

    [[nodiscard]]
    const Missile& missile() const noexcept;

    [[nodiscard]]
    const config::WorldConfig worldConfig() const noexcept;

    [[nodiscard]]
    bool isMissileCollision() const;

    [[nodiscard]]
    bool isOutOfBound() const;

    [[nodiscard]]
    std::uint64_t tick() const noexcept;

    void advance(Input);
    void reset(std::optional<unsigned int> seed = std::nullopt);
};

}