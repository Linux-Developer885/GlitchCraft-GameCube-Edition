#ifndef GLITCHCRAFT_GAME_ENGINE_TEXTURES_HPP
#define GLITCHCRAFT_GAME_ENGINE_TEXTURES_HPP

#include "glitchcraft/game_api.h"

#include <cstdint>

namespace glitchcraft::gameengine {

enum class TextureId : std::uint8_t {
    GrassTop,
    GrassSide,
    Dirt,
    Stone,
    Wood,
    WoodEnd,
    Water,
    Lava,
    Redstone,
    Rail,
    Tie,
    Solid,
    Count
};

struct TextureColor {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};

inline TextureId blockTexture(GcBlock block, int, int normalY, int)
{
    switch (block) {
    case GC_BLOCK_GRASS:
        if (normalY > 0) {
            return TextureId::GrassTop;
        }
        return normalY < 0 ? TextureId::Dirt : TextureId::GrassSide;
    case GC_BLOCK_DIRT: return TextureId::Dirt;
    case GC_BLOCK_STONE: return TextureId::Stone;
    case GC_BLOCK_WOOD:
        return normalY != 0 ? TextureId::WoodEnd : TextureId::Wood;
    case GC_BLOCK_WATER: return TextureId::Water;
    case GC_BLOCK_LAVA: return TextureId::Lava;
    case GC_BLOCK_REDSTONE: return TextureId::Redstone;
    default: return TextureId::Stone;
    }
}

inline TextureColor sampleTexture(TextureId texture, int u, int v)
{
    u &= 15;
    v &= 15;
    const std::uint32_t noise =
        static_cast<std::uint32_t>((u * 37 + v * 57 +
                                    static_cast<int>(texture) * 97) *
                                   (u + 3) * (v + 5));
    const int variation = static_cast<int>((noise ^ (noise >> 7U)) % 25U) - 12;
    TextureColor color{};
    switch (texture) {
    case TextureId::GrassTop: {
        const bool blade = ((u * 3 + v * 5 + static_cast<int>(noise)) % 13) == 0;
        color = blade ? TextureColor{77, 137, 48}
                      : TextureColor{92, 157, 57};
        break;
    }
    case TextureId::GrassSide:
        if (v < 4) {
            color = TextureColor{94, 164, 57};
        } else if ((u + v * 3) % 17 == 0) {
            color = TextureColor{81, 126, 47};
        } else {
            color = TextureColor{139, 91, 52};
        }
        break;
    case TextureId::Dirt:
        color = ((u * 5 + v * 7) % 19 == 0)
            ? TextureColor{112, 71, 42}
            : TextureColor{139, 91, 52};
        break;
    case TextureId::Stone:
        color = (u % 7 == 0 || v % 6 == 0)
            ? TextureColor{103, 108, 113}
            : TextureColor{133, 137, 141};
        break;
    case TextureId::Wood:
        color = (u % 5 == 0)
            ? TextureColor{112, 73, 39}
            : TextureColor{157, 111, 61};
        break;
    case TextureId::WoodEnd: {
        const int dx = u - 7;
        const int dy = v - 7;
        const int radius = dx * dx + dy * dy;
        color = ((radius / 5) % 2 == 0)
            ? TextureColor{178, 132, 78}
            : TextureColor{132, 91, 49};
        break;
    }
    case TextureId::Water:
        color = ((u + v * 2) % 7 < 2)
            ? TextureColor{89, 167, 225}
            : TextureColor{46, 112, 196};
        break;
    case TextureId::Lava:
        color = ((u * 3 + v * 5) % 11 < 3)
            ? TextureColor{255, 205, 67}
            : (((u + v * 2) % 7 == 0)
                   ? TextureColor{195, 48, 16}
                   : TextureColor{239, 91, 24});
        break;
    case TextureId::Redstone:
        color = ((u * 7 + v * 11) % 13 < 3)
            ? TextureColor{247, 97, 80}
            : TextureColor{128, 27, 31};
        break;
    case TextureId::Rail:
        color = (v < 3 || v > 12)
            ? TextureColor{201, 208, 212}
            : TextureColor{133, 143, 150};
        break;
    case TextureId::Tie:
        color = (u % 5 == 0)
            ? TextureColor{82, 52, 34}
            : TextureColor{119, 78, 47};
        break;
    case TextureId::Solid:
        color = TextureColor{255, 255, 255};
        break;
    case TextureId::Count:
        color = TextureColor{255, 0, 255};
        break;
    }
    if (texture == TextureId::Water || texture == TextureId::Lava) {
        return color;
    }
    const auto adjust = [variation](std::uint8_t channel) {
        const int value = static_cast<int>(channel) + variation;
        return static_cast<std::uint8_t>(value < 0 ? 0 : value > 255 ? 255
                                                                     : value);
    };
    return TextureColor{adjust(color.red), adjust(color.green),
                        adjust(color.blue)};
}

}

#endif
