#ifndef GLITCHCRAFT_GAME_ENGINE_WINDOW_HPP
#define GLITCHCRAFT_GAME_ENGINE_WINDOW_HPP

#include "renderer.hpp"

#include <memory>
#include <string>
#include <vector>

namespace glitchcraft::gameengine {

class Window {
public:
    Window();
    ~Window();
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    bool open(int width, int height);
    void close();
    bool isOpen() const noexcept;
    bool pollKey(int &key);
    void present(const Renderer &renderer, const Game &game);
    void drawOverlay(const std::string &title,
                     const std::vector<std::string> &lines);

private:
    struct Platform;
    std::unique_ptr<Platform> platform_;
};

}

#endif
