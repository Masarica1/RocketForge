#include <vector>
#include <string>

#include "vec2.hpp"

namespace simulation {
    std::vector<Vec2> getPolygon(const std::string& path, Vec2 sourceSize, Vec2 targetSize);
}