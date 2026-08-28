#include <tuple>

#include <raylib.h>

namespace simulation::frontend {


std::tuple<bool, bool, bool> getActionFromKeyboard() {
    return {IsKeyDown(KEY_A), IsKeyDown(KEY_W), IsKeyDown(KEY_D)};
}


}