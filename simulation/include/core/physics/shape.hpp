#pragma once
#include <vector>

#include "core/math/vec2.hpp"

namespace simulation {

/// Polygon data.
using Polygon = std::vector<Vec2>;

/// Polygon data derived from raw json data.
struct ShapeData {
    Polygon polygon;
    SizeInt sourceSize;

    Polygon toWolrdPolygon(SizeInt entitySize) const {
        return toWolrdPolygon(entitySize.toVec());
    }

    Polygon toWolrdPolygon(Vec2 entitySize) const {
        Polygon result = {};
        Vec2 ratio = {
            .x = entitySize.x / static_cast<float>(sourceSize.width),
            .y = entitySize.y / static_cast<float>(sourceSize.height)
        };

        for (const auto& vertice: polygon) {
            result.push_back(vertice * ratio);
        }
        return result;
    }
};

struct PolygonLibrary {
    Polygon rocket;
    Polygon missile;
};

}