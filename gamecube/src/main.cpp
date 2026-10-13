#include "engine.hpp"
#include "GameEngine/gx_renderer.hpp"

#include <gccore.h>
#include <ogc/pad.h>
#include <ogc/system.h>

#include <chrono>
#include <cmath>
#include <cstdint>

namespace {

constexpr float kStickDeadZone = 0.16F;
constexpr float kLookScale = 0.011F;
constexpr auto kDoubleTapWindow = std::chrono::milliseconds(320);

float normalizeAxis(s8 value)
{
    const float axis = static_cast<float>(value) / 128.0F;
    return std::abs(axis) < kStickDeadZone ? 0.0F : axis;
}

}

int main()
{
    PAD_Init();
    glitchcraft::gameengine::GxRenderer renderer;
    if (!renderer.initialize()) {
        return 1;
    }

    glitchcraft::Game game(
        static_cast<std::uint32_t>(std::chrono::steady_clock::now()
                                       .time_since_epoch()
                                       .count()));
    GcBlock selectedBlock = GC_BLOCK_GRASS;
    GcTrackType selectedTrack = GC_TRACK_STRAIGHT;
    GcVehicleType selectedVehicle = GC_VEHICLE_BIG_BOY;
    bool trackAxisX = true;
    bool inventoryOpen = false;
    bool running = true;
    int selectedSlot = 0;
    int buildMode = 0;
    auto lastForwardTap = std::chrono::steady_clock::time_point::min();
    bool wasMovingForward = false;
    const GcBlock blocks[5]{
        GC_BLOCK_GRASS, GC_BLOCK_DIRT, GC_BLOCK_STONE,
        GC_BLOCK_WOOD, GC_BLOCK_REDSTONE};

    while (running) {
        PAD_ScanPads();
        const u32 down = PAD_ButtonsDown(PAD_CHAN0);
        const u32 held = PAD_ButtonsHeld(PAD_CHAN0);
        const float forward =
            normalizeAxis(PAD_StickY(PAD_CHAN0));
        const float strafe =
            normalizeAxis(PAD_StickX(PAD_CHAN0));
        const float lookX =
            normalizeAxis(PAD_SubStickX(PAD_CHAN0));
        const float lookY =
            normalizeAxis(PAD_SubStickY(PAD_CHAN0));

        const auto now = std::chrono::steady_clock::now();
        const bool movingForward = forward > 0.5F;
        const bool sprint = movingForward && !wasMovingForward &&
            lastForwardTap != std::chrono::steady_clock::time_point::min() &&
            now - lastForwardTap <= kDoubleTapWindow;
        if (movingForward && !wasMovingForward) {
            lastForwardTap = now;
        }
        wasMovingForward = movingForward;
        game.move(forward, strafe, sprint);
        game.look(lookX * kLookScale, lookY * kLookScale);

        if ((down & PAD_BUTTON_START) != 0U) {
            game.setPaused(!game.paused());
        }
        if (!game.paused()) {
            if ((down & PAD_BUTTON_A) != 0U) {
                if (buildMode == 2) {
                    game.toggleTrainPower();
                } else {
                    game.jump();
                }
            }
            if ((down & PAD_BUTTON_X) != 0U) {
                inventoryOpen = !inventoryOpen;
            }
            if ((down & PAD_BUTTON_B) != 0U) {
                inventoryOpen = false;
            }
            if ((down & PAD_TRIGGER_L) != 0U) {
                if (buildMode == 1) {
                    game.placeTrack(selectedTrack, trackAxisX);
                } else if (buildMode == 2) {
                    game.placeVehicle(selectedVehicle);
                } else {
                    game.placeBlock(selectedBlock);
                }
            }
            if ((down & PAD_TRIGGER_R) != 0U) {
                game.mineBlock();
            }
            if ((down & PAD_BUTTON_UP) != 0U) {
                if (inventoryOpen) {
                    game.addTrainFuel(100);
                } else {
                    game.setFlying(!game.flying());
                }
            }
            if ((down & PAD_BUTTON_Y) != 0U) {
                if (inventoryOpen) {
                    game.toggleTrackSwitch();
                } else if (buildMode == 1) {
                    trackAxisX = !trackAxisX;
                } else {
                    buildMode = (buildMode + 1) % 3;
                }
            }
            if ((down & PAD_BUTTON_RIGHT) != 0U && !inventoryOpen) {
                if (buildMode == 0) {
                    selectedSlot = (selectedSlot + 1) % 5;
                    selectedBlock = blocks[selectedSlot];
                } else if (buildMode == 1) {
                    selectedTrack = static_cast<GcTrackType>(
                        (static_cast<int>(selectedTrack) + 1) % GC_TRACK_COUNT);
                } else {
                    selectedVehicle = static_cast<GcVehicleType>(
                        (static_cast<int>(selectedVehicle) + 1) %
                        GC_VEHICLE_COUNT);
                }
            }
            if ((down & PAD_BUTTON_LEFT) != 0U && !inventoryOpen) {
                if (buildMode == 0) {
                    selectedSlot = (selectedSlot + 4) % 5;
                    selectedBlock = blocks[selectedSlot];
                } else if (buildMode == 1) {
                    selectedTrack = static_cast<GcTrackType>(
                        (static_cast<int>(selectedTrack) + GC_TRACK_COUNT - 1) %
                        GC_TRACK_COUNT);
                } else {
                    selectedVehicle = static_cast<GcVehicleType>(
                        (static_cast<int>(selectedVehicle) +
                         GC_VEHICLE_COUNT - 1) % GC_VEHICLE_COUNT);
                }
            }
            if (inventoryOpen) {
                if ((down & PAD_BUTTON_LEFT) != 0U) {
                    game.drinkMilk();
                } else if ((down & PAD_BUTTON_RIGHT) != 0U) {
                    game.drinkPotion();
                }
            }
            game.tick();
        }

        renderer.render(game, selectedBlock, game.paused(), inventoryOpen,
                        selectedTrack, selectedVehicle, buildMode == 2);
        if ((held & PAD_BUTTON_B) != 0U && game.paused()) {
            game.setPaused(false);
        }
    }
    return 0;
}
