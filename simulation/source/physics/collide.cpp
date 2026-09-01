#include <cmath>
#include <vector>
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

    for (const auto& edge: edges) {
        Vec2 normalVector = {edge.y, -edge.x};

        std::vector<float> projectedA = {};
        projectedA.reserve(polygon1.size());
        for (const auto& vertice: polygon1) {
            projectedA.push_back(dot(normalVector, vertice));
        }

        std::vector<float> projectedB = {};
        projectedB.reserve(polygon2.size());
        for (const auto& vertice : polygon2) {
            projectedB.push_back(dot(normalVector, vertice));
        }

        auto minmaxA = std::minmax_element(projectedA.begin(), projectedA.end());
        float minA = *(minmaxA.first);
        float maxA = *(minmaxA.second);

        auto minmaxB = std::minmax_element(projectedB.begin(), projectedB.end());
        float minB = *(minmaxB.first);
        float maxB = *(minmaxB.second);

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
