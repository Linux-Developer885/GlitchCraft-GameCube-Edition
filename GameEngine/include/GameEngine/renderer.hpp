#ifndef GLITCHCRAFT_RENDERER_HPP
#define GLITCHCRAFT_RENDERER_HPP

#include "engine.hpp"
#include "GameEngine/textures.hpp"

#include <cstdint>
#include <vector>

namespace glitchcraft::gameengine {

class Renderer {
public:
    Renderer(int width, int height);

    void render(const Game &game);
    const std::vector<std::uint32_t> &pixels() const noexcept;
    int width() const noexcept;
    int height() const noexcept;

private:
    struct Vertex {
        float x;
        float y;
        float z;
        float u = 0.0F;
        float v = 0.0F;
    };

    int width_;
    int height_;
    float focalLength_;
    std::vector<std::uint32_t> pixels_;
    std::vector<float> depth_;

    void drawTriangle(const Vertex &a, const Vertex &b, const Vertex &c,
                      std::uint32_t color,
                      TextureId texture = TextureId::Count);
    void drawWorld(const Game &game);
};

}

#endif
