#include "GameEngine/renderer.hpp"
#include "GameEngine/vehicles.hpp"

#include "GameEngine/voxel_kernels.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace glitchcraft::gameengine {
namespace {

constexpr float kNearPlane = 0.08F;
constexpr int kViewDistance = 20;
constexpr float kPi = 3.14159265358979323846F;

struct Face {
    std::array<std::array<float, 3>, 4> corners;
    std::array<float, 3> normal;
};

constexpr std::array<Face, 6> kCubeFaces{{
    {{{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}}, {{0, 0, -1}}},
    {{{{1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}}}, {{0, 0, 1}}},
    {{{{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}}}, {{-1, 0, 0}}},
    {{{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}}, {{1, 0, 0}}},
    {{{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}}, {{0, 1, 0}}},
    {{{{0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1}}}, {{0, -1, 0}}},
}};

std::uint32_t packColor(int red, int green, int blue)
{
    return (static_cast<std::uint32_t>(red) << 16U) |
           (static_cast<std::uint32_t>(green) << 8U) |
           static_cast<std::uint32_t>(blue);
}

float dot3(const std::array<float, 3> &lhs,
           const std::array<float, 3> &rhs)
{
    const float left[4]{lhs[0], lhs[1], lhs[2], 0.0F};
    const float right[4]{rhs[0], rhs[1], rhs[2], 0.0F};
    return gc_engine_dot4(left, right);
}

}

Renderer::Renderer(int width, int height)
    : width_(std::max(1, width)),
      height_(std::max(1, height)),
      focalLength_(static_cast<float>(std::max(1, width)) * 0.86F),
      pixels_(static_cast<std::size_t>(width_) *
                  static_cast<std::size_t>(height_),
              0U),
      depth_(pixels_.size(), 0.0F)
{
}

void Renderer::render(const Game &game)
{
    const float skyBlend = std::clamp(
        0.5F + std::sin(game.pitch()) * 0.4F, 0.0F, 1.0F);

    for (int y = 0; y < height_; ++y) {
        const float vertical = static_cast<float>(y) /
                               static_cast<float>(height_);
        for (int x = 0; x < width_; ++x) {
            const float horizontal = static_cast<float>(x) /
                                    static_cast<float>(width_);
            const bool sky = vertical < 0.47F - skyBlend * 0.24F;
            const int fade = static_cast<int>(
                12.0F * std::sin(horizontal * kPi) *
                std::sin(vertical * kPi));
            pixels_[static_cast<std::size_t>(y) *
                        static_cast<std::size_t>(width_) +
                    static_cast<std::size_t>(x)] =
                sky ? packColor(104 + fade, 173 + fade, 233 + fade)
                    : packColor(102 + fade, 132 + fade, 74 + fade);
        }
    }
    gc_engine_clear_depth(depth_.data(), depth_.size(), 0.0F);
    drawWorld(game);
}

void Renderer::drawWorld(const Game &game)
{
    const float cameraX = game.playerX();
    const float cameraY = game.playerY() + 1.45F;
    const float cameraZ = game.playerZ();
    const std::array<float, 3> forward{
        std::cos(game.yaw()) * std::cos(game.pitch()),
        std::sin(game.pitch()),
        std::sin(game.yaw()) * std::cos(game.pitch())};
    const std::array<float, 3> right{
        -std::sin(game.yaw()), 0.0F, std::cos(game.yaw())};
    const std::array<float, 3> up{
        -std::cos(game.yaw()) * std::sin(game.pitch()),
        std::cos(game.pitch()),
        -std::sin(game.yaw()) * std::sin(game.pitch())};
    const std::array<float, 3> sun{-0.40F, 0.87F, -0.28F};

    const int centerX = static_cast<int>(std::floor(cameraX));
    const int centerZ = static_cast<int>(std::floor(cameraZ));
    const int minY = std::max(0, static_cast<int>(std::floor(cameraY)) - 9);
    const int maxY = static_cast<int>(std::floor(cameraY)) + 10;

    for (int blockX = centerX - kViewDistance;
         blockX <= centerX + kViewDistance; ++blockX) {
        for (int blockZ = centerZ - kViewDistance;
             blockZ <= centerZ + kViewDistance; ++blockZ) {
            const int dx = blockX - centerX;
            const int dz = blockZ - centerZ;
            if (dx * dx + dz * dz > kViewDistance * kViewDistance) {
                continue;
            }
            for (int blockY = minY; blockY <= maxY; ++blockY) {
                const GcBlock block = game.blockAt(blockX, blockY, blockZ);
                if (block == GC_BLOCK_AIR) {
                    continue;
                }
                for (const auto &face : kCubeFaces) {
                    const int adjacentX = blockX +
                        static_cast<int>(face.normal[0]);
                    const int adjacentY = blockY +
                        static_cast<int>(face.normal[1]);
                    const int adjacentZ = blockZ +
                        static_cast<int>(face.normal[2]);
                    const GcBlock neighbor =
                        game.blockAt(adjacentX, adjacentY, adjacentZ);
                    if (neighbor != GC_BLOCK_AIR && neighbor != GC_BLOCK_WATER &&
                        neighbor != GC_BLOCK_LAVA) {
                        continue;
                    }

                    const std::array<float, 3> toCamera{
                        cameraX - (static_cast<float>(blockX) + 0.5F),
                        cameraY - (static_cast<float>(blockY) + 0.5F),
                        cameraZ - (static_cast<float>(blockZ) + 0.5F)};
                    if (dot3(face.normal, toCamera) <= 0.0F) {
                        continue;
                    }

                    const float brightness = std::clamp(
                        0.40F + 0.60F *
                            std::max(0.0F, dot3(face.normal, sun)),
                        0.30F, 1.0F);
                    const int light = static_cast<int>(255.0F * brightness);
                    const std::uint32_t color = packColor(light, light, light);
                    const TextureId texture = blockTexture(
                        block, static_cast<int>(face.normal[0]),
                        static_cast<int>(face.normal[1]),
                        static_cast<int>(face.normal[2]));

                    std::array<Vertex, 4> vertices{};
                    bool clipped = false;
                    for (std::size_t corner = 0; corner < vertices.size(); ++corner) {
                        const float worldX = static_cast<float>(blockX) +
                                             face.corners[corner][0] - cameraX;
                        const float worldY = static_cast<float>(blockY) +
                                             face.corners[corner][1] - cameraY;
                        const float worldZ = static_cast<float>(blockZ) +
                                             face.corners[corner][2] - cameraZ;
                        const std::array<float, 3> relative{
                            worldX, worldY, worldZ};
                        const float textureU = face.normal[1] != 0
                            ? face.corners[corner][0]
                            : (face.normal[0] != 0
                                   ? face.corners[corner][2]
                                   : face.corners[corner][0]);
                        const float textureV = face.normal[1] != 0
                            ? face.corners[corner][2]
                            : face.corners[corner][1];
                        vertices[corner] = Vertex{
                            dot3(relative, right),
                            dot3(relative, up),
                            dot3(relative, forward), textureU, textureV};
                        clipped = clipped || vertices[corner].z <= kNearPlane;
                    }
                    if (clipped) {
                        continue;
                    }
                    drawTriangle(vertices[0], vertices[1], vertices[2], color,
                                 texture);
                    drawTriangle(vertices[0], vertices[2], vertices[3], color,
                                 texture);
                }
            }
        }
    }

    const auto drawBox = [&](float x, float y, float z,
                                 float sizeX, float sizeY, float sizeZ,
                                 std::uint32_t color,
                                 TextureId texture = TextureId::Count) {
            const float left = x - sizeX * 0.5F;
            const float rightEdge = x + sizeX * 0.5F;
            const float bottom = y - sizeY * 0.5F;
            const float top = y + sizeY * 0.5F;
            const float back = z - sizeZ * 0.5F;
            const float frontEdge = z + sizeZ * 0.5F;
            const std::array<std::array<float, 3>, 8> corners{{
                {{left, bottom, back}}, {{rightEdge, bottom, back}},
                {{rightEdge, top, back}}, {{left, top, back}},
                {{left, bottom, frontEdge}}, {{rightEdge, bottom, frontEdge}},
                {{rightEdge, top, frontEdge}}, {{left, top, frontEdge}}}};
            constexpr int faces[6][4]{{0, 3, 2, 1}, {4, 5, 6, 7},
                                      {0, 4, 7, 3}, {1, 2, 6, 5},
                                      {3, 7, 6, 2}, {0, 1, 5, 4}};
            for (const auto &face : faces) {
                Vertex projected[4]{};
                bool clipped = false;
                for (int index = 0; index < 4; ++index) {
                    const auto &corner = corners[static_cast<std::size_t>(
                        face[index])];
                    const std::array<float, 3> relative{
                        corner[0] - cameraX, corner[1] - cameraY,
                        corner[2] - cameraZ};
                    constexpr float uv[4][2]{{0.0F, 0.0F}, {1.0F, 0.0F},
                                             {1.0F, 1.0F}, {0.0F, 1.0F}};
                    projected[index] = Vertex{
                        dot3(relative, right), dot3(relative, up),
                        dot3(relative, forward), uv[index][0], uv[index][1]};
                    clipped = clipped || projected[index].z <= kNearPlane;
                }
                if (!clipped) {
                    drawTriangle(projected[0], projected[1], projected[2], color,
                                 texture);
                    drawTriangle(projected[0], projected[2], projected[3], color,
                                 texture);
                }
            }
        };

    for (const auto &entry : game.tracks()) {
            const Track &track = entry.second;
            const float x = static_cast<float>(track.position.x) + 0.5F;
            const float y = static_cast<float>(track.position.y);
            const float z = static_cast<float>(track.position.z) + 0.5F;
            if (std::abs(x - cameraX) > kViewDistance ||
                std::abs(z - cameraZ) > kViewDistance) {
                continue;
            }
            drawBox(x, y + 0.045F, z, 0.25F, 0.09F, 0.92F,
                    packColor(255, 255, 255), TextureId::Tie);
            const std::uint32_t railColor = packColor(157, 164, 169);
            if (track.axisX || track.type == GC_TRACK_INTERSECTION) {
                drawBox(x, y + 0.165F, z - 0.31F, 1.0F, 0.07F, 0.09F,
                        railColor, TextureId::Rail);
                drawBox(x, y + 0.165F, z + 0.31F, 1.0F, 0.07F, 0.09F,
                        railColor, TextureId::Rail);
            }
            if (!track.axisX || track.type == GC_TRACK_INTERSECTION) {
                drawBox(x - 0.31F, y + 0.165F, z, 0.09F, 0.07F, 1.0F,
                        railColor, TextureId::Rail);
                drawBox(x + 0.31F, y + 0.165F, z, 0.09F, 0.07F, 1.0F,
                        railColor, TextureId::Rail);
            }
            if (track.type == GC_TRACK_HILL) {
                drawBox(x, y + 0.26F, z, track.axisX ? 1.0F : 0.09F,
                        0.07F, track.axisX ? 0.09F : 1.0F, railColor,
                        TextureId::Rail);
            } else if (track.type == GC_TRACK_SWITCH && track.switchBranch) {
                drawBox(x - 0.10F, y + 0.20F, z, 0.09F, 0.07F, 0.74F,
                        railColor, TextureId::Rail);
            } else if (track.type == GC_TRACK_TURNTABLE) {
                drawBox(x, y + 0.18F, z, 0.92F, 0.06F, 0.12F,
                        packColor(255, 255, 255), TextureId::Rail);
            } else if (track.type == GC_TRACK_CURVED ||
                       track.type == GC_TRACK_CUSTOM_CURVE) {
                drawBox(x + 0.12F, y + 0.17F, z - 0.22F,
                        0.55F, 0.07F, 0.09F, railColor);
            }
        }

    for (const TrainVehicle &vehicle : game.vehicles()) {
            const float x = static_cast<float>(vehicle.trackPosition.x) + 0.5F;
            const float y = static_cast<float>(vehicle.trackPosition.y) + 0.20F;
            const float z = static_cast<float>(vehicle.trackPosition.z) + 0.5F;
            if (std::abs(x - cameraX) > kViewDistance ||
                std::abs(z - cameraZ) > kViewDistance) {
                continue;
            }
            const bool locomotive = isLocomotive(vehicle.type);
            const bool tender = isSteamTender(vehicle.type);
            const bool passenger = isPassengerVehicle(vehicle.type);
            const bool shinkansen =
                isShinkansenEngine(vehicle.type) ||
                isShinkansenCoach(vehicle.type);
            const float length = locomotive ? 0.98F : 0.92F;
            const float width = 0.55F;
            const float lengthX = vehicle.facingX ? length : width;
            const float lengthZ = vehicle.facingX ? width : length;
            const auto body = vehicleColor(vehicle.type);
            const std::uint32_t bodyColor =
                packColor(body.red, body.green, body.blue);
            const std::uint32_t windowColor = packColor(32, 43, 54);
            drawBox(x, y + 0.08F, z, lengthX, 0.15F, lengthZ,
                    packColor(47, 49, 52));
            if (locomotive) {
                drawBox(x, y + 0.43F, z, lengthX * 0.82F, 0.56F,
                        lengthZ * 0.90F, bodyColor);
                drawBox(x, y + 0.73F, z, lengthX * 0.64F, 0.10F,
                        lengthZ * 0.96F, packColor(47, 49, 52));
                if (isSteamLocomotive(vehicle.type)) {
                    drawBox(x, y + 0.81F, z, 0.20F, 0.28F, 0.20F,
                            packColor(47, 49, 52));
                    if (vehicle.type == GC_VEHICLE_FLYING_SCOTSMAN) {
                        drawBox(x, y + 0.55F, z, lengthX * 0.52F, 0.29F,
                                lengthZ * 0.55F, bodyColor);
                        drawBox(x, y + 0.78F, z, 0.18F, 0.22F, 0.18F,
                                packColor(28, 29, 27));
                    }
                } else if (shinkansen) {
                    const float noseOffset = -static_cast<float>(
                        vehicle.direction) * length * 0.47F;
                    drawBox(x + (vehicle.facingX ? noseOffset : 0.0F),
                            y + 0.30F,
                            z + (vehicle.facingX ? 0.0F : noseOffset),
                            vehicle.facingX ? 0.32F : 0.40F, 0.20F,
                            vehicle.facingX ? 0.40F : 0.32F, bodyColor);
                    drawBox(x, y + 0.55F, z,
                            lengthX * 0.54F, 0.12F, lengthZ * 0.78F,
                            windowColor);
                }
            } else if (tender) {
                drawBox(x, y + 0.33F, z, lengthX * 0.84F, 0.38F,
                        lengthZ * 0.90F, bodyColor);
            } else {
                drawBox(x, y + 0.34F, z, lengthX * 0.86F, 0.48F,
                        lengthZ * 0.88F, bodyColor);
                drawBox(x, y + 0.61F, z, lengthX * 0.88F, 0.08F,
                        lengthZ * 0.94F, packColor(47, 49, 52));
                if (passenger) {
                    drawBox(x, y + 0.43F, z,
                            lengthX * 0.76F, 0.10F, lengthZ * 0.92F,
                            bodyColor);
                    drawBox(x, y + 0.49F, z,
                            lengthX * 0.74F, 0.16F, lengthZ * 0.84F,
                            windowColor);
                    if (shinkansen) {
                        drawBox(x, y + 0.70F, z,
                                lengthX * 0.80F, 0.07F, lengthZ * 0.94F,
                                bodyColor);
                    }
                } else if (vehicle.type == GC_VEHICLE_TANK_CAR) {
                    drawBox(x, y + 0.42F, z, lengthX * 0.68F, 0.34F,
                            lengthZ * 0.65F, packColor(183, 180, 165));
                } else if (vehicle.type == GC_VEHICLE_AUTORACK_CAR) {
                    drawBox(x, y + 0.36F, z, lengthX * 0.89F, 0.52F,
                            lengthZ * 0.12F, packColor(47, 49, 52));
                } else if (vehicle.type == GC_VEHICLE_CABOOSE) {
                    drawBox(x, y + 0.73F, z, 0.23F, 0.15F, 0.35F,
                            bodyColor);
                }
            }
            const std::uint32_t wheelColor = packColor(28, 28, 30);
            for (const float offset : {-0.27F, 0.27F}) {
                for (const float side : {-0.20F, 0.20F}) {
                    const float wheelX = x + (vehicle.facingX ? offset : side);
                    const float wheelZ = z + (vehicle.facingX ? side : offset);
                    drawBox(wheelX, y + 0.07F, wheelZ,
                            0.15F, 0.14F, 0.15F, wheelColor);
                }
            }
            for (const float end : {-1.0F, 1.0F}) {
                const float offset = end * (length * 0.5F + 0.08F);
                drawBox(x + (vehicle.facingX ? offset : 0.0F), y + 0.07F,
                        z + (vehicle.facingX ? 0.0F : offset),
                        vehicle.facingX ? 0.15F : 0.08F, 0.10F,
                        vehicle.facingX ? 0.08F : 0.15F,
                        packColor(75, 75, 78));
            }
        }
    for (const RemotePlayer &player : game.remotePlayers()) {
            if (std::abs(player.x - cameraX) > kViewDistance ||
                std::abs(player.z - cameraZ) > kViewDistance) {
                continue;
            }
            const float playerX = player.x;
            const float playerY = player.y;
            const float playerZ = player.z;
            drawBox(playerX, playerY + 0.45F, playerZ,
                    0.52F, 0.86F, 0.30F, packColor(44, 116, 191));
            drawBox(playerX, playerY + 1.10F, playerZ,
                    0.34F, 0.34F, 0.34F, packColor(204, 155, 112));
            drawBox(playerX - 0.15F, playerY - 0.12F, playerZ,
                    0.20F, 0.30F, 0.26F, packColor(52, 62, 91));
            drawBox(playerX + 0.15F, playerY - 0.12F, playerZ,
                    0.20F, 0.30F, 0.26F, packColor(52, 62, 91));
    }
}

void Renderer::drawTriangle(const Vertex &a, const Vertex &b,
                            const Vertex &c, std::uint32_t color,
                            TextureId texture)
{
    const auto projectX = [this](const Vertex &vertex) {
        return static_cast<float>(width_) * 0.5F +
               focalLength_ * vertex.x / vertex.z;
    };
    const auto projectY = [this](const Vertex &vertex) {
        return static_cast<float>(height_) * 0.5F -
               focalLength_ * vertex.y / vertex.z;
    };

    const float ax = projectX(a);
    const float ay = projectY(a);
    const float bx = projectX(b);
    const float by = projectY(b);
    const float cx = projectX(c);
    const float cy = projectY(c);
    const float area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    if (std::abs(area) < 0.0001F) {
        return;
    }
    const int minX = std::max(0, static_cast<int>(
        std::floor(std::min({ax, bx, cx}))));
    const int maxX = std::min(width_ - 1, static_cast<int>(
        std::ceil(std::max({ax, bx, cx}))));
    const int minY = std::max(0, static_cast<int>(
        std::floor(std::min({ay, by, cy}))));
    const int maxY = std::min(height_ - 1, static_cast<int>(
        std::ceil(std::max({ay, by, cy}))));
    if (minX > maxX || minY > maxY) {
        return;
    }

    const float inverseA = 1.0F / a.z;
    const float inverseB = 1.0F / b.z;
    const float inverseC = 1.0F / c.z;
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float px = static_cast<float>(x) + 0.5F;
            const float py = static_cast<float>(y) + 0.5F;
            const float weightA =
                ((bx - px) * (cy - py) - (by - py) * (cx - px)) / area;
            const float weightB =
                ((cx - px) * (ay - py) - (cy - py) * (ax - px)) / area;
            const float weightC = 1.0F - weightA - weightB;
            if (weightA < -0.001F || weightB < -0.001F ||
                weightC < -0.001F) {
                continue;
            }
            const float inverseDepth =
                weightA * inverseA + weightB * inverseB + weightC * inverseC;
            const std::size_t index =
                static_cast<std::size_t>(y) *
                    static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(x);
            if (inverseDepth > depth_[index]) {
                depth_[index] = inverseDepth;
                if (texture == TextureId::Count) {
                    pixels_[index] = color;
                } else {
                    const float u = (weightA * a.u * inverseA +
                                     weightB * b.u * inverseB +
                                     weightC * c.u * inverseC) / inverseDepth;
                    const float v = (weightA * a.v * inverseA +
                                     weightB * b.v * inverseB +
                                     weightC * c.v * inverseC) / inverseDepth;
                    const TextureColor texel = sampleTexture(
                        texture, static_cast<int>(std::floor(u * 16.0F)),
                        static_cast<int>(std::floor(v * 16.0F)));
                    const auto tint = (color >> 16U) & 0xffU;
                    const auto tintGreen = (color >> 8U) & 0xffU;
                    const auto tintBlue = color & 0xffU;
                    pixels_[index] = packColor(
                        static_cast<int>(texel.red * tint / 255U),
                        static_cast<int>(texel.green * tintGreen / 255U),
                        static_cast<int>(texel.blue * tintBlue / 255U));
                }
            }
        }
    }
}

const std::vector<std::uint32_t> &Renderer::pixels() const noexcept
{
    return pixels_;
}

int Renderer::width() const noexcept { return width_; }
int Renderer::height() const noexcept { return height_; }

}
