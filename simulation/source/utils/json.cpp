#include <vector>
#include <string>
#include <fstream>

#include <nlohmann/json.hpp>

#include "json.hpp"
#include "vec2.hpp"

namespace simulation {

std::vector<Vec2> getPolygon(const std::string& path, Vec2 sourceSize, Vec2 targetSize) {
    std::vector<Vec2> polygon = {};

    std::ifstream file(path);
    nlohmann::json data;

    file >> data;

    for (const auto& vertex : data["vertices"]) {
        polygon.push_back({
            vertex["x"].get<float>() * targetSize.x / sourceSize.x,
            (sourceSize.y - vertex["y"].get<float>()) * targetSize.y / sourceSize.y
        });
    }
    return polygon;
}

}

