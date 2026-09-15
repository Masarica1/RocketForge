#pragma once
#include "core/physics/transform.hpp"
#include "core/physics/shape.hpp"
#include "core/math/vec2.hpp"
#include "transform.hpp"

namespace simulation {

bool isOutOfBound(const Transform&, SizeInt spaceSize);

bool isCollide(
    const Transform&,
    const Polygon&,
    const Transform&,
    const Polygon&
);


}