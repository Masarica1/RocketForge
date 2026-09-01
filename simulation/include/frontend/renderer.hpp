#pragma once
#include <array>
#include <cassert>
#include <tuple>

#include <raylib.h>

#include "simulation.hpp"
#include "transform.hpp"
#include "vec2.hpp"

namespace simulation::frontend {

namespace  {

class ResourceManager {
private:
    std::array<Texture2D, 8> rocketTextures = std::array{
        LoadTexture("asset/rocket/rocket-000.png"),
        LoadTexture("asset/rocket/rocket-001.png"),
        LoadTexture("asset/rocket/rocket-010.png"),
        LoadTexture("asset/rocket/rocket-011.png"),
        LoadTexture("asset/rocket/rocket-100.png"),
        LoadTexture("asset/rocket/rocket-101.png"),
        LoadTexture("asset/rocket/rocket-110.png"),
        LoadTexture("asset/rocket/rocket-111.png"),
    };
    Texture2D missileTexture = LoadTexture("asset/missile/missile.png");
    Texture2D bgTexture = LoadTexture("asset/background/background.png");

    ResourceManager() = default;

    ~ResourceManager() {
        for(const Texture2D& t: rocketTextures) {
            UnloadTexture(t);
        }
        UnloadTexture(bgTexture);
        UnloadTexture(missileTexture);
    }

public:
    static ResourceManager& get() {
        static ResourceManager manager;
        return manager;
    }

    Texture2D getRocketTexture(int index) {
        assert((0 <= index) && (index < 8));
        return rocketTextures[index];
    }

    Texture2D getMissileTexture() {
        return missileTexture;
    }

    Texture2D getBackgroundTexture() {
        return bgTexture;
    }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator = (const ResourceManager&) = delete;

};

std::tuple<Rectangle, float> convertCoordination(Size windowSize, Size spaceSize, const simulation::Transform& transform) {
    float magnification = static_cast<float>(windowSize.width) / static_cast<float>(spaceSize.width);

    float x = transform.pos.x * magnification;
    float y =(static_cast<float>(spaceSize.height) - transform.top()) * magnification;
    float anlge = - transform.angle * RAD2DEG;

    Vec2 size = transform.size * magnification;
    return {Rectangle(x, y, size.x, size.y), anlge};
}

void drawEntity(Rectangle destination, float angle, Texture2D texture) {
    Rectangle source = {0.0f, 0.0f, (float) texture.width, (float) texture.height};
    Vector2 origin = {destination.width / 2, destination.height / 2};

    DrawTexturePro(texture, source, destination, origin, angle, WHITE);
}

}


void render(
    Size windowSize,
    const Simulation& sim,
    const std::tuple<bool, bool, bool>& action
) {
    assert(windowSize.width * 9 == windowSize.height * 16);

    BeginDrawing();;

    auto drawer = [windowSize, spaceSize = sim.spaceSize()](const Transform& transform, Texture2D texture) {
        auto [rectDest, angle] = convertCoordination(windowSize, spaceSize, transform);
        drawEntity(rectDest, angle, texture);
    };

    // draw background
    Texture2D backgroundTexture = ResourceManager::get().getBackgroundTexture();
    DrawTexturePro(
        backgroundTexture,
        {0., 0., (float) backgroundTexture.width, (float) backgroundTexture.height},
        {0., 0., (float) windowSize.width, (float) windowSize.height},
        {0., 0.},
        0,
        WHITE
    );

    // draw rocket
    auto [leftAct, mainAct, rightAct] = action;
    Texture2D rocketTexture = ResourceManager::get().getRocketTexture(
        4*static_cast<int>(leftAct)+2*static_cast<int>(mainAct)+static_cast<int>(rightAct)
    );

    drawer(sim.rocket().transform(), rocketTexture);
    if (sim.missile().alive()) {
        drawer(sim.missile().transform(), ResourceManager::get().getMissileTexture());
    }
    
    EndDrawing();
}


}