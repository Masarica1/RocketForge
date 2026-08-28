#pragma once
#include <array>
#include <cassert>
#include <tuple>

#include <raylib.h>

#include "simulation.hpp"
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
    Texture2D bgTexture = LoadTexture("asset/background/background.png");

    ResourceManager() = default;

    ~ResourceManager() {
        for(const Texture2D& t: rocketTextures) {
            UnloadTexture(t);
        }
        UnloadTexture(bgTexture);
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

    Texture2D getBackgroundTexture() {
        return bgTexture;
    }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator = (const ResourceManager&) = delete;

};

}


void render(
    Size windowSize,
    const Simulation& sim,
    const std::tuple<bool, bool, bool>& action
) {
    assert(windowSize.width * 9 == windowSize.height * 16);

    BeginDrawing();;

    float magnification = static_cast<float>(windowSize.width) / static_cast<float>(sim.spaceSize().width);
    Texture2D backgroundTexture = ResourceManager::get().getBackgroundTexture();

    // draw background
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

    float rocketX = sim.rocket().transform().pos.x * magnification;
    float rocketY = (static_cast<float>(sim.spaceSize().height) - sim.rocket().transform().top()) * magnification;
    float rocketAngle = -sim.rocket().transform().angle * RAD2DEG;
    Vec2 rocketSize = sim.rocket().transform().size * magnification;
    Vec2 rocketCenter = Vec2(rocketX, rocketY) + rocketSize / 2;

    Rectangle rocketSource = {0., 0., (float) rocketTexture.width, (float) rocketTexture.height};
    Rectangle rocketDest = {rocketX, rocketY, rocketSize.x, rocketSize.y};
    DrawTexturePro(rocketTexture, rocketSource, rocketDest, {rocketSize.x/2, rocketSize.y/2}, rocketAngle, WHITE);

    EndDrawing();
}


}