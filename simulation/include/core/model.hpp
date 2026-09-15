#pragma once

namespace simulation {

enum class WorldEvent {
    OutOfBound = 0,
    MissileCollision = 1
};

struct Input {
    bool left;
    bool main;
    bool right;
};

}

