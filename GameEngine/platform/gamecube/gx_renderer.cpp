#include "GameEngine/gx_renderer.hpp"
#include "GameEngine/vehicles.hpp"

#include <gccore.h>
#include <ogc/system.h>
#include <ogc/video.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace glitchcraft::gameengine {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr int kWorldHeight = 18;
constexpr float kWheelRadius = 0.13F;

struct Face {
    std::array<std::array<float, 3>, 4> corners;
    std::array<int, 3> normal;
};

constexpr std::array<Face, 6> kFaces{{
    {{{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}}, {{0, 0, -1}}},
    {{{{1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}}}, {{0, 0, 1}}},
    {{{{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}}}, {{-1, 0, 0}}},
    {{{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}}, {{1, 0, 0}}},
    {{{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}}, {{0, 1, 0}}},
    {{{{0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1}}}, {{0, -1, 0}}},
}};

GXRModeObj *videoMode;
void *framebuffer;
alignas(32) std::uint16_t textureAtlas[1024U * 16U]{};

GXColor blockColor(const std::array<int, 3> &normal)
{
    float brightness = 0.56F;
    if (normal[1] > 0) {
        brightness = 1.0F;
    } else if (normal[1] < 0) {
        brightness = 0.48F;
    } else if (normal[0] != 0) {
        brightness = 0.76F;
    } else if (normal[2] > 0) {
        brightness = 0.86F;
    }
    const auto light = static_cast<u8>(255.0F * brightness);
    return GXColor{
        light, light, light, 255};
}

GXColor gxVehicleColor(GcVehicleType type)
{
    const VehicleColor color = gameengine::vehicleColor(type);
    return GXColor{color.red, color.green, color.blue, 255};
}

void emitVertex(float x, float y, float z, TextureId texture,
                float u, float v)
{
    GX_Position3f32(x, y, z);
    const float atlasU =
        (static_cast<float>(texture) * 16.0F + 0.5F + u * 15.0F) /
        1024.0F;
    const float atlasV = (0.5F + v * 15.0F) / 16.0F;
    GX_TexCoord2f32(atlasU, atlasV);
}

}

alignas(32) std::uint8_t GxRenderer::fifo_[256U * 1024U]{};

bool GxRenderer::initialize()
{
    VIDEO_Init();
    videoMode = VIDEO_GetPreferredMode(nullptr);
    if (videoMode == nullptr) {
        return false;
    }

    framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(videoMode));
    if (framebuffer == nullptr) {
        return false;
    }
    VIDEO_Configure(videoMode);
    VIDEO_SetNextFramebuffer(framebuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if ((videoMode->viTVMode & VI_NON_INTERLACE) != 0U) {
        VIDEO_WaitVSync();
    }

    GX_Init(fifo_, sizeof(fifo_));
    GX_SetCopyClear(GXColor{112, 180, 239, 255}, 0x00ffffffU);
    GX_SetViewport(0.0F, 0.0F,
                   static_cast<f32>(videoMode->fbWidth),
                   static_cast<f32>(videoMode->efbHeight), 0.0F, 1.0F);
    GX_SetScissor(0, 0, videoMode->fbWidth, videoMode->efbHeight);
    GX_SetDispCopyYScale(GX_GetYScaleFactor(videoMode->efbHeight,
                                            videoMode->xfbHeight));
    GX_SetDispCopySrc(0, 0, videoMode->fbWidth, videoMode->efbHeight);
    GX_SetDispCopyDst(videoMode->fbWidth, videoMode->xfbHeight);
    GX_SetCopyFilter(videoMode->aa, videoMode->sample_pattern,
                     GX_TRUE, videoMode->vfilter);
    GX_SetFieldMode(videoMode->field_rendering,
                    static_cast<u8>(videoMode->viHeight ==
                                            2U * videoMode->xfbHeight
                                        ? GX_ENABLE
                                        : GX_DISABLE));
    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetCullMode(GX_CULL_BACK);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_TRUE);

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetNumChans(1);
    GX_SetNumTexGens(1);
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0,
                   GX_COLOR0A0);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX,
                   GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    buildTextureAtlas();
    GX_InitTexObj(&textureObject_, textureAtlas, 1024, 16, GX_TF_RGB5A3,
                  GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjFilterMode(&textureObject_, GX_NEAR, GX_NEAR);
    GX_LoadTexObj(&textureObject_, GX_TEXMAP0);
    GX_InvVtxCache();
    GX_InvalidateTexAll();
    GX_SetDispCopyGamma(GX_GM_1_0);
    return true;
}

void GxRenderer::buildTextureAtlas()
{
    for (std::size_t textureIndex = 0;
         textureIndex < static_cast<std::size_t>(TextureId::Count);
         ++textureIndex) {
        const auto texture = static_cast<TextureId>(textureIndex);
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                const TextureColor color = sampleTexture(texture, x, y);
                const std::uint16_t packed = static_cast<std::uint16_t>(
                    0x8000U |
                    (static_cast<std::uint16_t>(color.red >> 3U) << 10U) |
                    (static_cast<std::uint16_t>(color.green >> 3U) << 5U) |
                    static_cast<std::uint16_t>(color.blue >> 3U));
                const std::size_t atlasX = textureIndex * 16U +
                                           static_cast<std::size_t>(x);
                const std::size_t tileBlock =
                    static_cast<std::size_t>(y / 4) * 256U + atlasX / 4U;
                const std::size_t tileOffset =
                    static_cast<std::size_t>(y % 4) * 4U +
                    atlasX % 4U;
                textureAtlas[tileBlock * 16U + tileOffset] = packed;
            }
        }
    }
    DCFlushRange(textureAtlas, sizeof(textureAtlas));
}

void GxRenderer::render(const Game &game, GcBlock selectedBlock,
                        bool paused, bool inventoryOpen,
                        GcTrackType selectedTrack,
                        GcVehicleType selectedVehicle, bool vehicleSelected,
                        bool shulkerOpen, Position shulkerPosition,
                        int activeShulkerSlot)
{
    GX_SetCopyClear(GXColor{112, 180, 239, 255}, 0x00ffffffU);
    GX_InvalidateTexAll();
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetCullMode(GX_CULL_BACK);
    GX_SetNumTexGens(1);
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0,
                   GX_COLOR0A0);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_LoadTexObj(&textureObject_, GX_TEXMAP0);

    const f32 width = static_cast<f32>(videoMode->fbWidth);
    const f32 height = static_cast<f32>(videoMode->efbHeight);
    Mtx projection;
    guPerspective(projection, 60.0F, width / height, 0.1F, 120.0F);
    GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);

    drawWorld(game);
    for (const auto &entry : game.tracks()) {
        drawTrack(entry.second);
    }
    for (const auto &vehicle : game.vehicles()) {
        drawVehicle(vehicle);
    }
    for (const RemotePlayer &player : game.remotePlayers()) {
        drawRemotePlayer(player);
    }

    GX_DrawDone();
    GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GX_SetCullMode(GX_CULL_NONE);
    Mtx identity;
    guMtxIdentity(identity);
    GX_LoadPosMtxImm(identity, GX_PNMTX0);
    GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
    drawHud(selectedBlock, paused, inventoryOpen, selectedTrack,
            selectedVehicle, vehicleSelected, game, shulkerOpen,
            shulkerPosition, activeShulkerSlot);

    GX_DrawDone();
    GX_CopyDisp(framebuffer, GX_TRUE);
    VIDEO_SetNextFramebuffer(framebuffer);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

void GxRenderer::drawWorld(const Game &game)
{
    const float cameraX = game.playerX();
    const float cameraY = game.playerY() + 1.45F;
    const float cameraZ = game.playerZ();
    const float yaw = game.yaw();
    const float pitch = game.pitch();
    guVector camera{cameraX, cameraY, cameraZ};
    guVector target{
        cameraX + std::cos(yaw) * std::cos(pitch),
        cameraY + std::sin(pitch),
        cameraZ + std::sin(yaw) * std::cos(pitch)};
    guVector worldUp{0.0F, 1.0F, 0.0F};
    Mtx view;
    guLookAt(view, &camera, &worldUp, &target);
    GX_LoadPosMtxImm(view, GX_PNMTX0);

    const int centerX = static_cast<int>(std::floor(cameraX));
    const int centerZ = static_cast<int>(std::floor(cameraZ));
    const int minY = std::max(
        0, static_cast<int>(std::floor(cameraY)) - kWorldHeight / 2);
    const int maxY = static_cast<int>(std::floor(cameraY)) + kWorldHeight / 2;

    for (int blockX = centerX - kViewDistance;
         blockX <= centerX + kViewDistance; ++blockX) {
        for (int blockZ = centerZ - kViewDistance;
             blockZ <= centerZ + kViewDistance; ++blockZ) {
            const int dx = blockX - centerX;
            const int dz = blockZ - centerZ;
            if (dx * dx + dz * dz > kViewDistance * kViewDistance) {
                continue;
            }
            u16 faceCount = 0;
            for (int blockY = minY; blockY <= maxY; ++blockY) {
                const GcBlock block = game.blockAt(blockX, blockY, blockZ);
                if (block == GC_BLOCK_AIR) {
                    continue;
                }
                for (const auto &face : kFaces) {
                    const int adjacentX = blockX + face.normal[0];
                    const int adjacentY = blockY + face.normal[1];
                    const int adjacentZ = blockZ + face.normal[2];
                    const GcBlock neighbor =
                        game.blockAt(adjacentX, adjacentY, adjacentZ);
                    if (neighbor != GC_BLOCK_AIR &&
                        neighbor != GC_BLOCK_WATER &&
                        neighbor != GC_BLOCK_LAVA) {
                        continue;
                    }
                    ++faceCount;
                }
            }
            if (faceCount == 0U) {
                continue;
            }
            GX_Begin(GX_QUADS, GX_VTXFMT0,
                     static_cast<u16>(faceCount * 4U));
            for (int blockY = minY; blockY <= maxY; ++blockY) {
                const GcBlock block = game.blockAt(blockX, blockY, blockZ);
                if (block == GC_BLOCK_AIR) {
                    continue;
                }
                for (const auto &face : kFaces) {
                    const int adjacentX = blockX + face.normal[0];
                    const int adjacentY = blockY + face.normal[1];
                    const int adjacentZ = blockZ + face.normal[2];
                    const GcBlock neighbor =
                        game.blockAt(adjacentX, adjacentY, adjacentZ);
                    if (neighbor != GC_BLOCK_AIR &&
                        neighbor != GC_BLOCK_WATER &&
                        neighbor != GC_BLOCK_LAVA) {
                        continue;
                    }
                    const GXColor color = blockColor(face.normal);
                    const TextureId texture = blockTexture(
                        block, face.normal[0], face.normal[1], face.normal[2]);
                    GX_Color4u8(color.r, color.g, color.b, color.a);
                    for (std::size_t cornerIndex = 0;
                         cornerIndex < face.corners.size(); ++cornerIndex) {
                        const auto &corner = face.corners[cornerIndex];
                        const float u = face.normal[1] != 0
                            ? corner[0]
                            : (face.normal[0] != 0 ? corner[2] : corner[0]);
                        const float v = face.normal[1] != 0
                            ? corner[2] : corner[1];
                        emitVertex(static_cast<float>(blockX) + corner[0],
                                   static_cast<float>(blockY) + corner[1],
                                   static_cast<float>(blockZ) + corner[2],
                                   texture, u, v);
                    }
                }
            }
            GX_End();
        }
    }
}

void GxRenderer::drawTrack(const Track &track)
{
    const float x = static_cast<float>(track.position.x) + 0.5F;
    const float y = static_cast<float>(track.position.y);
    const float z = static_cast<float>(track.position.z) + 0.5F;
    const GXColor tie{77, 54, 38, 255};
    const GXColor rail{157, 164, 169, 255};
    const auto bar = [](float left, float bottom, float back,
                        float right, float top, float front, GXColor color,
                        TextureId texture) {
        const std::array<std::array<float, 3>, 8> corners{{
            {{left, bottom, back}}, {{right, bottom, back}},
            {{right, top, back}}, {{left, top, back}},
            {{left, bottom, front}}, {{right, bottom, front}},
            {{right, top, front}}, {{left, top, front}}}};
        constexpr int faces[6][4]{{0, 3, 2, 1}, {4, 5, 6, 7},
                                  {0, 4, 7, 3}, {1, 2, 6, 5},
                                  {3, 7, 6, 2}, {0, 1, 5, 4}};
        for (const auto &face : faces) {
            GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
            GX_Color4u8(color.r, color.g, color.b, color.a);
            constexpr float uv[4][2]{{0.0F, 0.0F}, {0.0F, 1.0F},
                                     {1.0F, 1.0F}, {1.0F, 0.0F}};
            for (int corner = 0; corner < 4; ++corner) {
                const int index = face[corner];
                emitVertex(corners[static_cast<std::size_t>(index)][0],
                           corners[static_cast<std::size_t>(index)][1],
                           corners[static_cast<std::size_t>(index)][2],
                           texture, uv[corner][0], uv[corner][1]);
            }
            GX_End();
        }
    };

    bar(x - 0.13F, y, z - 0.46F, x + 0.13F, y + 0.10F,
        z + 0.46F, tie, TextureId::Tie);
    if (track.type == GC_TRACK_HILL) {
        bar(x - 0.34F, y + 0.10F, z - 0.5F, x - 0.24F,
            y + 0.19F, z + 0.5F, rail, TextureId::Rail);
        bar(x + 0.24F, y + 0.10F, z - 0.5F, x + 0.34F,
            y + 0.19F, z + 0.5F, rail, TextureId::Rail);
        return;
    }
    if (track.axisX || track.type == GC_TRACK_INTERSECTION) {
        bar(x - 0.50F, y + 0.13F, z - 0.36F,
            x + 0.50F, y + 0.20F, z - 0.27F, rail, TextureId::Rail);
        bar(x - 0.50F, y + 0.13F, z + 0.27F,
            x + 0.50F, y + 0.20F, z + 0.36F, rail, TextureId::Rail);
    }
    if (!track.axisX || track.type == GC_TRACK_INTERSECTION) {
        bar(x - 0.36F, y + 0.13F, z - 0.50F,
            x - 0.27F, y + 0.20F, z + 0.50F, rail, TextureId::Rail);
        bar(x + 0.27F, y + 0.13F, z - 0.50F,
            x + 0.36F, y + 0.20F, z + 0.50F, rail, TextureId::Rail);
    }
    if (track.type == GC_TRACK_CURVED ||
        track.type == GC_TRACK_CUSTOM_CURVE) {
        const float bend = track.type == GC_TRACK_CUSTOM_CURVE ? 0.45F : 0.25F;
        bar(x - 0.36F, y + 0.13F, z - 0.50F,
            x - 0.27F, y + 0.20F, z - 0.12F, rail, TextureId::Rail);
        bar(x - 0.27F, y + 0.13F, z - 0.20F,
            x - 0.08F, y + 0.20F, z - 0.12F, rail, TextureId::Rail);
        bar(x - 0.08F, y + 0.13F, z - 0.20F,
            x + bend, y + 0.20F, z - 0.12F, rail, TextureId::Rail);
    }
    if (track.type == GC_TRACK_SWITCH) {
        bar(x - 0.34F, y + 0.14F, z - 0.42F,
            x + 0.34F, y + 0.19F, z - 0.34F, rail, TextureId::Rail);
        if (track.switchBranch) {
            bar(x - 0.11F, y + 0.16F, z - 0.34F,
                x - 0.03F, y + 0.22F, z + 0.42F, rail, TextureId::Rail);
        }
    }
    if (track.type == GC_TRACK_TURNTABLE) {
        bar(x - 0.46F, y + 0.13F, z - 0.06F,
            x + 0.46F, y + 0.20F, z + 0.06F,
            GXColor{207, 172, 62, 255}, TextureId::Rail);
    }
}

void GxRenderer::drawVehicle(const TrainVehicle &vehicle)
{
    const float x = static_cast<float>(vehicle.trackPosition.x) + 0.5F;
    const float y = static_cast<float>(vehicle.trackPosition.y) + 0.20F;
    const float z = static_cast<float>(vehicle.trackPosition.z) + 0.5F;
    const bool locomotive = isLocomotive(vehicle.type);
    const bool tender = isSteamTender(vehicle.type);
    const bool passenger = isPassengerVehicle(vehicle.type);
    const bool shinkansen =
        isShinkansenEngine(vehicle.type) ||
        isShinkansenCoach(vehicle.type);
    const float length = locomotive ? 0.98F : (tender ? 0.88F : 0.92F);
    const float width = locomotive ? 0.58F : 0.55F;
    const float alongSign = static_cast<float>(vehicle.direction);
    const GXColor body = gxVehicleColor(vehicle.type);
    const GXColor roof{47, 49, 52, 255};
    const GXColor wheels{28, 28, 30, 255};
    const GXColor coupler{75, 75, 78, 255};
    const auto box = [&](float centerX, float centerY, float centerZ,
                         float extentX, float extentY, float extentZ,
                         GXColor color, TextureId texture = TextureId::Solid) {
        const float left = centerX - extentX * 0.5F;
        const float right = centerX + extentX * 0.5F;
        const float bottom = centerY - extentY * 0.5F;
        const float top = centerY + extentY * 0.5F;
        const float back = centerZ - extentZ * 0.5F;
        const float front = centerZ + extentZ * 0.5F;
        const std::array<std::array<float, 3>, 8> vertices{{
            {{left, bottom, back}}, {{right, bottom, back}},
            {{right, top, back}}, {{left, top, back}},
            {{left, bottom, front}}, {{right, bottom, front}},
            {{right, top, front}}, {{left, top, front}}}};
        constexpr int faces[6][4]{{0, 3, 2, 1}, {4, 5, 6, 7},
                                  {0, 4, 7, 3}, {1, 2, 6, 5},
                                  {3, 7, 6, 2}, {0, 1, 5, 4}};
        for (const auto &face : faces) {
            GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
            GX_Color4u8(color.r, color.g, color.b, color.a);
            constexpr float uv[4][2]{{0.0F, 0.0F}, {0.0F, 1.0F},
                                     {1.0F, 1.0F}, {1.0F, 0.0F}};
            for (int corner = 0; corner < 4; ++corner) {
                const int index = face[corner];
                emitVertex(vertices[static_cast<std::size_t>(index)][0],
                           vertices[static_cast<std::size_t>(index)][1],
                           vertices[static_cast<std::size_t>(index)][2],
                           texture, uv[corner][0], uv[corner][1]);
            }
            GX_End();
        }
    };

    box(x, y + 0.08F, z, length, 0.15F, width, roof);
    if (locomotive) {
        box(x, y + 0.43F, z, length * 0.78F, 0.56F,
            width * 0.88F, body);
        box(x, y + 0.73F, z, length * 0.64F, 0.10F,
            width * 0.96F, roof);
        if (isSteamLocomotive(vehicle.type)) {
            box(x, y + 0.55F, z, length * 0.52F, 0.29F,
                width * 0.55F, body);
            box(x + (vehicle.facingX ? alongSign * 0.13F : 0.0F),
                y + 0.77F, z + (vehicle.facingX ? 0.0F : 0.13F),
                0.20F, 0.28F, 0.20F, roof);
        } else {
            box(x + (vehicle.facingX ? alongSign * 0.13F : 0.0F),
                y + 0.77F, z + (vehicle.facingX ? 0.0F : 0.13F),
                0.20F, 0.28F, 0.20F, roof);
        }
        if (shinkansen) {
            const float noseOffset = -alongSign * length * 0.47F;
            box(x + (vehicle.facingX ? noseOffset : 0.0F), y + 0.30F,
                z + (vehicle.facingX ? 0.0F : noseOffset),
                vehicle.facingX ? 0.32F : 0.40F, 0.20F,
                vehicle.facingX ? 0.40F : 0.32F, body);
            box(x, y + 0.55F, z, length * 0.54F, 0.12F,
                width * 0.78F, GXColor{32, 43, 54, 255});
        }
    } else if (tender) {
        box(x, y + 0.33F, z, length * 0.84F, 0.38F,
            width * 0.90F, body);
        box(x, y + 0.54F, z, length * 0.70F, 0.07F,
            width * 0.96F, roof);
    } else {
        box(x, y + 0.34F, z, length * 0.86F, 0.48F,
            width * 0.88F, body);
        box(x, y + 0.61F, z, length * 0.88F, 0.08F,
            width * 0.94F, roof);
        if (passenger) {
            box(x, y + 0.47F, z, length * 0.76F, 0.18F,
                width * 0.84F, GXColor{32, 43, 54, 255});
            if (shinkansen) {
                box(x, y + 0.70F, z, length * 0.80F, 0.07F,
                    width * 0.94F, body);
            }
        } else if (vehicle.type == GC_VEHICLE_HOPPER_CAR) {
            box(x, y + 0.10F, z, length * 0.60F, 0.18F,
                width * 0.72F, body);
        } else if (vehicle.type == GC_VEHICLE_TANK_CAR) {
            box(x, y + 0.40F, z, length * 0.68F, 0.34F,
                width * 0.65F, roof);
        } else if (vehicle.type == GC_VEHICLE_AUTORACK_CAR) {
            for (int section = -1; section <= 1; ++section) {
                box(x, y + 0.36F, z + static_cast<float>(section) * 0.15F,
                    length * 0.89F, 0.52F, 0.045F, roof);
            }
        } else if (vehicle.type == GC_VEHICLE_CABOOSE) {
            box(x, y + 0.73F, z, 0.23F, 0.15F,
                width * 0.70F, body);
        }
    }

    for (const float along : {-length * 0.31F, length * 0.31F}) {
        const float wheelX = vehicle.facingX ? x + along * alongSign : x;
        const float wheelZ = vehicle.facingX ? z : z + along;
        for (const float across : {-width * 0.39F, width * 0.39F}) {
            box(wheelX + (vehicle.facingX ? 0.0F : across),
                y + kWheelRadius * 0.5F,
                wheelZ + (vehicle.facingX ? across : 0.0F),
                0.16F, 0.14F, 0.16F, wheels);
        }
    }
    const float couplerOffset = length * 0.5F + 0.08F;
    for (const float end : {-1.0F, 1.0F}) {
        const float offset = end * couplerOffset *
            (vehicle.facingX ? alongSign : 1.0F);
        box(x + (vehicle.facingX ? offset : 0.0F), y + 0.07F,
            z + (vehicle.facingX ? 0.0F : offset),
            vehicle.facingX ? 0.15F : 0.08F, 0.10F,
            vehicle.facingX ? 0.08F : 0.15F, coupler);
    }
}

void GxRenderer::drawRemotePlayer(const RemotePlayer &player)
{
    const auto box = [](float x, float y, float z, float sizeX,
                        float sizeY, float sizeZ, GXColor color) {
        const float left = x - sizeX * 0.5F;
        const float right = x + sizeX * 0.5F;
        const float bottom = y - sizeY * 0.5F;
        const float top = y + sizeY * 0.5F;
        const float back = z - sizeZ * 0.5F;
        const float front = z + sizeZ * 0.5F;
        const std::array<std::array<float, 3>, 8> vertices{{
            {{left, bottom, back}}, {{right, bottom, back}},
            {{right, top, back}}, {{left, top, back}},
            {{left, bottom, front}}, {{right, bottom, front}},
            {{right, top, front}}, {{left, top, front}}}};
        constexpr int faces[6][4]{{0, 3, 2, 1}, {4, 5, 6, 7},
                                  {0, 4, 7, 3}, {1, 2, 6, 5},
                                  {3, 7, 6, 2}, {0, 1, 5, 4}};
        for (const auto &face : faces) {
            GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
            GX_Color4u8(color.r, color.g, color.b, color.a);
            constexpr float uv[4][2]{{0.0F, 0.0F}, {0.0F, 1.0F},
                                     {1.0F, 1.0F}, {1.0F, 0.0F}};
            for (int corner = 0; corner < 4; ++corner) {
                const int index = face[corner];
                emitVertex(vertices[static_cast<std::size_t>(index)][0],
                           vertices[static_cast<std::size_t>(index)][1],
                           vertices[static_cast<std::size_t>(index)][2],
                           TextureId::Solid, uv[corner][0], uv[corner][1]);
            }
            GX_End();
        }
    };
    const GXColor shirt{44, 116, 191, 255};
    const GXColor skin{204, 155, 112, 255};
    const GXColor pants{52, 62, 91, 255};
    box(player.x, player.y + 0.45F, player.z, 0.52F, 0.86F,
        0.30F, shirt);
    box(player.x, player.y + 1.10F, player.z, 0.34F, 0.34F,
        0.34F, skin);
    box(player.x - 0.15F, player.y - 0.12F, player.z,
        0.20F, 0.30F, 0.26F, pants);
    box(player.x + 0.15F, player.y - 0.12F, player.z,
        0.20F, 0.30F, 0.26F, pants);
}

void GxRenderer::drawQuad(
    const std::array<std::array<float, 2>, 4> &vertices, GXColor color)
{
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Color4u8(color.r, color.g, color.b, color.a);
    for (const auto &vertex : vertices) {
        GX_Position3f32(vertex[0], vertex[1], 0.0F);
    }
    GX_End();
}

void GxRenderer::drawHud(GcBlock selectedBlock, bool paused,
                         bool inventoryOpen, GcTrackType selectedTrack,
                         GcVehicleType selectedVehicle, bool vehicleSelected,
                         const Game &game, bool shulkerOpen,
                         Position shulkerPosition, int activeShulkerSlot)
{
    GX_SetNumTexGens(0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL,
                   GX_COLOR0A0);
    GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
    Mtx projection;
    const f32 width = static_cast<f32>(videoMode->fbWidth);
    const f32 height = static_cast<f32>(videoMode->efbHeight);
    guOrtho(projection, 0.0F, height, 0.0F, width, -1.0F, 1.0F);
    GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);

    const float centerX = width * 0.5F;
    const float centerY = height * 0.5F;
    const GXColor white{255, 255, 255, 255};
    drawQuad({{{centerX - 8.0F, centerY - 1.0F},
               {centerX + 8.0F, centerY - 1.0F},
               {centerX + 8.0F, centerY + 1.0F},
               {centerX - 8.0F, centerY + 1.0F}}}, white);
    drawQuad({{{centerX - 1.0F, centerY - 8.0F},
               {centerX + 1.0F, centerY - 8.0F},
               {centerX + 1.0F, centerY + 8.0F},
               {centerX - 1.0F, centerY + 8.0F}}}, white);

    const float slotSize = 38.0F;
    const float gap = 3.0F;
    const float barWidth = 9.0F * slotSize + 8.0F * gap;
    const float startX = (width - barWidth) * 0.5F;
    const float top = height - 52.0F;
    for (int slot = 0; slot < 9; ++slot) {
        const float x = startX + static_cast<float>(slot) * (slotSize + gap);
        drawQuad({{{x, top}, {x + slotSize, top},
                   {x + slotSize, top + slotSize}, {x, top + slotSize}}},
                 GXColor{20, 20, 20, 220});
        drawQuad({{{x + 2.0F, top + 2.0F},
                   {x + slotSize - 2.0F, top + 2.0F},
                   {x + slotSize - 2.0F, top + slotSize - 2.0F},
                   {x + 2.0F, top + slotSize - 2.0F}}},
                 slot == 0 && !vehicleSelected
                     ? [&selectedBlock]() {
                           const auto selectedTexture = blockTexture(
                               selectedBlock, 0, 1, 0);
                           const auto sample = sampleTexture(
                               selectedTexture, 8, 8);
                           return GXColor{sample.red, sample.green,
                                          sample.blue, 255};
                       }()
                     : (slot == 1 && !vehicleSelected
                            ? GXColor{137, 143, 149, 255}
                            : (slot == 2 && vehicleSelected
                                   ? gxVehicleColor(selectedVehicle)
                                   : GXColor{92, 92, 92, 255})));
    }
    if (vehicleSelected) {
        const GXColor color = gxVehicleColor(selectedVehicle);
        drawQuad({{{startX, top - 7.0F}, {startX + slotSize, top - 7.0F},
                   {startX + slotSize, top - 3.0F}, {startX, top - 3.0F}}},
                 color);
    } else if (selectedTrack >= GC_TRACK_STRAIGHT &&
               selectedTrack < GC_TRACK_COUNT) {
        const GXColor rail{185, 191, 197, 255};
        drawQuad({{{startX, top - 7.0F}, {startX + slotSize, top - 7.0F},
                   {startX + slotSize, top - 3.0F}, {startX, top - 3.0F}}},
                 rail);
    }

    if (paused || inventoryOpen) {
        const float panelWidth = std::min(width - 32.0F, 400.0F);
        const float panelHeight = paused ? 110.0F :
                                  (shulkerOpen ? 210.0F : 190.0F);
        const float left = (width - panelWidth) * 0.5F;
        const float topPanel = (height - panelHeight) * 0.5F;
        drawQuad({{{left, topPanel}, {left + panelWidth, topPanel},
                   {left + panelWidth, topPanel + panelHeight},
                   {left, topPanel + panelHeight}}},
                 GXColor{20, 24, 34, 238});
        const GXColor accent{83, 166, 255, 255};
        drawQuad({{{left + 12.0F, topPanel + 12.0F},
                   {left + panelWidth - 12.0F, topPanel + 12.0F},
                   {left + panelWidth - 12.0F, topPanel + 17.0F},
                   {left + 12.0F, topPanel + 17.0F}}}, accent);
        if (inventoryOpen) {
            if (shulkerOpen) {
                constexpr float cell = 30.0F;
                constexpr float gap = 3.0F;
                const float gridWidth = 9.0F * cell + 8.0F * gap;
                const float gridLeft = (width - gridWidth) * 0.5F;
                const float gridTop = topPanel + 34.0F;
                for (int slotIndex = 0; slotIndex < 27; ++slotIndex) {
                    const int column = slotIndex % 9;
                    const int row = slotIndex / 9;
                    const float x = gridLeft +
                        static_cast<float>(column) * (cell + gap);
                    const float y = gridTop +
                        static_cast<float>(row) * (cell + gap);
                    drawQuad({{{x, y}, {x + cell, y},
                               {x + cell, y + cell}, {x, y + cell}}},
                             slotIndex == activeShulkerSlot
                                 ? GXColor{255, 233, 111, 255}
                                 : GXColor{87, 92, 101, 255});
                    glitchcraft::ShulkerStack stack;
                    game.getShulkerSlot(
                        shulkerPosition.x, shulkerPosition.y,
                        shulkerPosition.z, slotIndex, stack);
                    GXColor swatch{38, 42, 48, 255};
                    if (stack.count > 0) {
                        const TextureColor color = sampleTexture(
                            blockTexture(stack.block, 0, 1, 0), 8, 8);
                        swatch = GXColor{color.red, color.green, color.blue,
                                         255};
                    }
                    drawQuad({{{x + 3.0F, y + 3.0F},
                               {x + cell - 3.0F, y + 3.0F},
                               {x + cell - 3.0F, y + cell - 3.0F},
                               {x + 3.0F, y + cell - 3.0F}}}, swatch);
                }
                return;
            }
            drawQuad({{{left + 24.0F, topPanel + 35.0F},
                       {left + panelWidth - 24.0F, topPanel + 35.0F},
                       {left + panelWidth - 24.0F, topPanel + 39.0F},
                       {left + 24.0F, topPanel + 39.0F}}}, white);
            drawQuad({{{left + 24.0F, topPanel + 53.0F},
                       {left + panelWidth - 56.0F, topPanel + 53.0F},
                       {left + panelWidth - 56.0F, topPanel + 57.0F},
                       {left + 24.0F, topPanel + 57.0F}}},
                     GXColor{105, 168, 66, 255});
            drawQuad({{{left + 24.0F, topPanel + 75.0F},
                       {left + panelWidth - 56.0F, topPanel + 75.0F},
                       {left + panelWidth - 56.0F, topPanel + 79.0F},
                       {left + 24.0F, topPanel + 79.0F}}},
                     GXColor{132, 135, 139, 255});
            drawQuad({{{left + 24.0F, topPanel + 97.0F},
                       {left + panelWidth - 56.0F, topPanel + 97.0F},
                       {left + panelWidth - 56.0F, topPanel + 101.0F},
                       {left + 24.0F, topPanel + 101.0F}}},
                     GXColor{195, 43, 39, 255});
        }
    }
}

}
