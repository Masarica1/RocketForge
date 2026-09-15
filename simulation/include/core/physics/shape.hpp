#pragma once
#include <vector>

#include "core/math/vec2.hpp"

namespace simulation {

/// Polygon data derived from asset json data.
using Polygon = std::vector<Vec2>;

struct ShapeData {
    Polygon polygon;
    SizeInt sourceSize;
};

struct ShapeLibaray {
    ShapeData rocket;
    ShapeData sourceSize;
};

}