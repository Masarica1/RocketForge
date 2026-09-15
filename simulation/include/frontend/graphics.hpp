#include <array>

#include <raylib.h>

namespace simulation::fronted {


struct ImageAssets {
    std::array<Texture2D, 8> rocket;
    Texture2D background;
    Texture2D missile;

    explicit ImageAssets();
};


}



