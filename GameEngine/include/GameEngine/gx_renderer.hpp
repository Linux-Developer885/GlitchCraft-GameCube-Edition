#ifndef GLITCHCRAFT_GAME_ENGINE_GX_RENDERER_HPP
#define GLITCHCRAFT_GAME_ENGINE_GX_RENDERER_HPP

#include "engine.hpp"
#include "GameEngine/textures.hpp"

#include <gccore.h>

#include <array>
#include <cstdint>

namespace glitchcraft::gameengine {

class GxRenderer {
public:
    bool initialize();
    void render(const Game &game, GcBlock selectedBlock, bool paused,
                bool inventoryOpen, GcTrackType selectedTrack,
                GcVehicleType selectedVehicle, bool vehicleSelected);

private:
    static constexpr int kViewDistance = 12;
    alignas(32) static std::uint8_t fifo_[256U * 1024U];
    GXTexObj textureObject_{};

    void drawWorld(const Game &game);
    void drawTrack(const Track &track);
    void drawVehicle(const TrainVehicle &vehicle);
    void drawRemotePlayer(const RemotePlayer &player);
    void drawHud(GcBlock selectedBlock, bool paused, bool inventoryOpen,
                 GcTrackType selectedTrack, GcVehicleType selectedVehicle,
                 bool vehicleSelected);
    void buildTextureAtlas();
    void drawQuad(const std::array<std::array<float, 2>, 4> &vertices,
                  GXColor color);
};

}

#endif
