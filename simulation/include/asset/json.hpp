#pragma once

#include <fstream>
#include <utility>
#include <string>

#include <nlohmann/json.hpp>

#include "core/physics/shape.hpp"

namespace simulation {

inline void from_json(const nlohmann::json& j, ShapeData& data) {
    ShapeData parsed;
    const auto& size = j.at("size");
    size.at("x").get_to(parsed.sourceSize.width);
    size.at("y").get_to(parsed.sourceSize.height);

    const auto& vertices = j.at("vertices").get_ref<const nlohmann::json::array_t&>();
    parsed.polygon.reserve(vertices.size());
    for (const auto& vertex : vertices) {
        parsed.polygon.push_back({
            vertex.at("x").get<float>(),
            vertex.at("y").get<float>()
        });
    }

    data = std::move(parsed);
}

inline ShapeData LoadShapeData(const std::string& path) {
    std::ifstream file(path);
    nlohmann::json j;
    file >> j;

    return j.get<ShapeData>();
}

}
