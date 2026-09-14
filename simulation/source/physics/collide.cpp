#include <cmath>
#include <vector>
#include <limits>
#include <algorithm>

#include "transform.hpp"
#include "vec2.hpp"
#include "collide.hpp"
namespace simulation {

namespace {

    void calculatePolygon(
        std::vector<Vec2>& container,
        const Transform& transform,
        const std::vector<Vec2>& polysonSource
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

}


bool isCollide(const std::vector<Vec2>& polygon1, const std::vector<Vec2>& polygon2) {
    if (polygon1.empty() || polygon2.empty()) return false;

    std::vector<Vec2> edges = {};
    auto addEdges = [&edges](const std::vector<Vec2>& polygon) {
        for (int i = 1; i < polygon.size(); i++) {
            edges.push_back(polygon[i]-polygon[i-1]);
        }
        edges.push_back(polygon[0] - polygon[polygon.size()-1]);
    };
    addEdges(polygon1);
    addEdges(polygon2);

    for (const Vec2& edge: edges) {
        Vec2 normalVector = {edge.y, -edge.x};

        float minA = std::numeric_limits<float>::max();
        float maxA = std::numeric_limits<float>::lowest();
        float minB = std::numeric_limits<float>::max();
        float maxB = std::numeric_limits<float>::lowest();

        for (const Vec2& vertice: polygon1) {
            float projectedVal = dot(normalVector, vertice);
            minA = std::min(minA, projectedVal);
            maxA = std::max(maxA, projectedVal);
        }
        for (const Vec2& vertice: polygon2) {
            float projectedVal = dot(normalVector, vertice);
            minB = std::min(minB, projectedVal);
            maxB = std::max(maxB, projectedVal);
        }

        if (maxA <= minB || maxB <= minA) return false;
    }

    return true;

}

bool isCollide(
    const Transform& transform1,
    const std::vector<Vec2>& polygonSource1,
    const Transform& transform2,
    const std::vector<Vec2>& polygonSource2
) {
    std::vector<Vec2> polygon1 = {};
    std::vector<Vec2> polygon2 = {};

    calculatePolygon(polygon1, transform1, polygonSource1);
    calculatePolygon(polygon2, transform2, polygonSource2);
    return isCollide(polygon1, polygon2);
}

}
