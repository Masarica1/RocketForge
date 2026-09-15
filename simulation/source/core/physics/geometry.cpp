#include <stdexcept>

#include "core/physics/geometry.hpp"
#include "core/math/vec2.hpp"
#include "core/physics/shape.hpp"
#include "core/physics/transform.hpp"

namespace simulation {


void calculatePolygon(
    Polygon& container,
    const Transform& transform,
    const Polygon& polysonSource
) {
    float angle = transform.angle;

    container.reserve(polysonSource.size());
    
    for (const Vec2& vertice: polysonSource) {
        Vec2 relativePos = vertice + (transform.pos - transform.center());
        Vec2 newPos = transform.center() + Vec2(
            dot(relativePos, Vec2(std::cos(angle), -std::sin(angle))),
            dot(relativePos, Vec2(std::sin(angle), std::cos(angle)))
        );
        container.push_back(newPos);
    }
}


bool isOutOfBound(const Transform& transform, SizeInt spaceSize) {
    auto spaceSizeVec = spaceSize.toVec();

    if (
        transform.top() < -0.5 * transform.size.y
        || transform.bottom() > spaceSizeVec.x + 0.5 * transform.size.y
        || transform.right() < -0.5 * transform.size.x
        || transform.left() > spaceSizeVec.y + 0.5 * transform.size.x
    ) {
        return true;
    }
    return false;
}


/// @param shape1, shape2 must have Resized polygon data. not raw. 
bool isCollide(
    const Transform& tf1,
    const Polygon& polygon1,
    const Transform& tf2,
    const Polygon& polygon2
) {
    if (polygon1.empty() || polygon2.empty()) {
        throw std::runtime_error("Polygon data must not empty.");
    }

    Polygon absPolygon1 = {};
    Polygon absPolygon2 = {};
    calculatePolygon(absPolygon1, tf1, polygon1);
    calculatePolygon(absPolygon2, tf2, polygon2);

    std::vector<Vec2> edges = {};
    auto addEdges = [&edges](const Transform& tf, const Polygon& polygon) {

        for (int i = 0; i < polygon.size(); i++) {
            edges.push_back(polygon[i] - polygon[(i + 1) % polygon.size()]);
        }
    };
    addEdges(tf1, absPolygon1);
    addEdges(tf2, absPolygon2);

    for (const Vec2& edge: edges) {
        Vec2 normalVector = {edge.y, -edge.x};

        float minA = std::numeric_limits<float>::max();
        float maxA = std::numeric_limits<float>::lowest();
        float minB = std::numeric_limits<float>::max();
        float maxB = std::numeric_limits<float>::lowest();

        for (const Vec2& vertice: absPolygon1) {
            float projectedVal = dot(normalVector, vertice);
            minA = std::min(minA, projectedVal);
            maxA = std::max(maxA, projectedVal);
        }
        for (const Vec2& vertice: absPolygon2) {
            float projectedVal = dot(normalVector, vertice);
            minB = std::min(minB, projectedVal);
            maxB = std::max(maxB, projectedVal);
        }

        if (maxA <= minB || maxB <= minA) return false;
    }

    return true;
}


}

