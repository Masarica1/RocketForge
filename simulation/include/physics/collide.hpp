#include <vector>

#include "transform.hpp"
#include "vec2.hpp"

namespace simulation {

bool isCollide(const std::vector<Vec2>& polygon1, const std::vector<Vec2>& polygon2);

bool isCollide(
    const Transform& transform1,
    const std::vector<Vec2>& polygonSource1,
    const Transform& transform2,
    const std::vector<Vec2>& polygonSource2
);

}

