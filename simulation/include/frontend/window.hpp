#include <string>

#include <raylib.h>

namespace simulation::frontend {

class Window {
private:
    inline static bool exists_ = false;

protected:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator = (const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator = (Window&&) = delete;
};

}