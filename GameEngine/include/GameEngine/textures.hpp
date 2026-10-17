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
    ShulkerWhite,
    ShulkerOrange,
    ShulkerMagenta,
    ShulkerLightBlue,
    ShulkerYellow,
    ShulkerLime,
    ShulkerPink,
    ShulkerGray,
    ShulkerLightGray,
    ShulkerCyan,
    ShulkerPurple,
    ShulkerBlue,
    ShulkerBrown,
    ShulkerGreen,
    ShulkerRed,
    ShulkerBlack,
    Rail,
    Tie,
    Solid,
    Asphalt,
    RoadStraight,
    RoadCurve,
    RoadTIntersection,
    RoadCrossIntersection,
    RoadDeadEnd,
    RoadRoundabout,
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
    case GC_BLOCK_SHULKER_WHITE: return TextureId::ShulkerWhite;
    case GC_BLOCK_SHULKER_ORANGE: return TextureId::ShulkerOrange;
    case GC_BLOCK_SHULKER_MAGENTA: return TextureId::ShulkerMagenta;
    case GC_BLOCK_SHULKER_LIGHT_BLUE: return TextureId::ShulkerLightBlue;
    case GC_BLOCK_SHULKER_YELLOW: return TextureId::ShulkerYellow;
    case GC_BLOCK_SHULKER_LIME: return TextureId::ShulkerLime;
    case GC_BLOCK_SHULKER_PINK: return TextureId::ShulkerPink;
    case GC_BLOCK_SHULKER_GRAY: return TextureId::ShulkerGray;
    case GC_BLOCK_SHULKER_LIGHT_GRAY: return TextureId::ShulkerLightGray;
    case GC_BLOCK_SHULKER_CYAN: return TextureId::ShulkerCyan;
    case GC_BLOCK_SHULKER_PURPLE: return TextureId::ShulkerPurple;
    case GC_BLOCK_SHULKER_BLUE: return TextureId::ShulkerBlue;
    case GC_BLOCK_SHULKER_BROWN: return TextureId::ShulkerBrown;
    case GC_BLOCK_SHULKER_GREEN: return TextureId::ShulkerGreen;
    case GC_BLOCK_SHULKER_RED: return TextureId::ShulkerRed;
    case GC_BLOCK_SHULKER_BLACK: return TextureId::ShulkerBlack;
    case GC_BLOCK_ASPHALT: return TextureId::Asphalt;
    case GC_BLOCK_ROAD_STRAIGHT:
        return normalY > 0 ? TextureId::RoadStraight : TextureId::Asphalt;
    case GC_BLOCK_ROAD_CURVE:
        return normalY > 0 ? TextureId::RoadCurve : TextureId::Asphalt;
    case GC_BLOCK_ROAD_T_INTERSECTION:
        return normalY > 0 ? TextureId::RoadTIntersection
                           : TextureId::Asphalt;
    case GC_BLOCK_ROAD_CROSS_INTERSECTION:
        return normalY > 0 ? TextureId::RoadCrossIntersection
                           : TextureId::Asphalt;
    case GC_BLOCK_ROAD_DEAD_END:
        return normalY > 0 ? TextureId::RoadDeadEnd : TextureId::Asphalt;
    case GC_BLOCK_ROAD_ROUNDABOUT:
        return normalY > 0 ? TextureId::RoadRoundabout : TextureId::Asphalt;
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
    case TextureId::ShulkerWhite:
    case TextureId::ShulkerOrange:
    case TextureId::ShulkerMagenta:
    case TextureId::ShulkerLightBlue:
    case TextureId::ShulkerYellow:
    case TextureId::ShulkerLime:
    case TextureId::ShulkerPink:
    case TextureId::ShulkerGray:
    case TextureId::ShulkerLightGray:
    case TextureId::ShulkerCyan:
    case TextureId::ShulkerPurple:
    case TextureId::ShulkerBlue:
    case TextureId::ShulkerBrown:
    case TextureId::ShulkerGreen:
    case TextureId::ShulkerRed:
    case TextureId::ShulkerBlack: {
        constexpr TextureColor palettes[]{
            {224, 220, 211}, {219, 119, 48}, {170, 65, 165},
            {105, 152, 190}, {226, 199, 55}, {112, 171, 35},
            {202, 112, 143}, {76, 80, 82}, {151, 151, 143},
            {45, 137, 143}, {126, 60, 167}, {51, 63, 143},
            {111, 77, 54}, {74, 107, 42}, {160, 48, 42},
            {37, 34, 38}};
        const int paletteIndex = static_cast<int>(texture) -
                                 static_cast<int>(TextureId::ShulkerWhite);
        const TextureColor base = palettes[paletteIndex];
        const bool rim = u < 2 || u > 13 || v < 2 || v > 13;
        const bool latch = u >= 6 && u <= 9 && v >= 6 && v <= 9;
        const int multiplier = rim ? 72 : (latch ? 128 : 100);
        color = TextureColor{
            static_cast<std::uint8_t>(static_cast<int>(base.red) *
                                      multiplier / 100),
            static_cast<std::uint8_t>(static_cast<int>(base.green) *
                                      multiplier / 100),
            static_cast<std::uint8_t>(static_cast<int>(base.blue) *
                                      multiplier / 100)};
        break;
    }
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
    case TextureId::Asphalt:
        color = ((u * 7 + v * 11) % 9 == 0)
            ? TextureColor{72, 76, 80}
            : TextureColor{57, 61, 65};
        break;
    case TextureId::RoadStraight:
    case TextureId::RoadCurve:
    case TextureId::RoadTIntersection:
    case TextureId::RoadCrossIntersection:
    case TextureId::RoadDeadEnd:
    case TextureId::RoadRoundabout: {
        const int dx = u - 7;
        const int dy = v - 7;
        bool marking = false;
        if (texture == TextureId::RoadStraight) {
            marking = (u == 7 || u == 8) &&
                      (v < 3 || (v >= 6 && v <= 9) || v > 12);
        } else if (texture == TextureId::RoadCurve) {
            marking = (dx * dx + dy * dy >= 26 &&
                       dx * dx + dy * dy <= 39 && u >= 7 && v <= 8);
        } else if (texture == TextureId::RoadTIntersection) {
            marking = ((u == 7 || u == 8) && v >= 7) ||
                      ((v == 7 || v == 8) && u >= 2 && u <= 13);
        } else if (texture == TextureId::RoadCrossIntersection) {
            marking = (u == 7 || u == 8 || v == 7 || v == 8);
        } else if (texture == TextureId::RoadDeadEnd) {
            marking = (u == 7 || u == 8) && v >= 1 && v <= 12;
            marking = marking || (v == 12 && u >= 3 && u <= 12);
        } else {
            const int radius = dx * dx + dy * dy;
            marking = radius >= 13 && radius <= 29;
        }
        if (marking) {
            color = TextureColor{225, 193, 74};
        } else {
            color = ((u * 7 + v * 11) % 9 == 0)
                ? TextureColor{72, 76, 80}
                : TextureColor{57, 61, 65};
        }
        break;
    }
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
