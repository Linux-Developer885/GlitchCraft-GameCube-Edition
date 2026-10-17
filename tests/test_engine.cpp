#include "engine.hpp"
#include "glitchcraft/game_api.h"
#include "GameEngine/renderer.hpp"
#include "GameEngine/textures.hpp"
#include "GameEngine/vehicles.hpp"
#include "GameEngine/voxel_kernels.h"
#include "realms.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <sstream>
#include <thread>

namespace {

int failures = 0;

void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

}

int main()
{
    constexpr float pi = 3.14159265358979323846F;
    glitchcraft::Game first(1234U);
    glitchcraft::Game sameSeed(1234U);
    glitchcraft::Game otherSeed(5678U);
    check(first.blockAt(4, 0, 6) == sameSeed.blockAt(4, 0, 6),
          "terrain generation is deterministic for a seed");
    check(first.blockAt(4, 0, 6) == GC_BLOCK_STONE,
          "terrain has a solid underground layer");
    check(first.blockAt(4000000, 0, -3000000) == GC_BLOCK_STONE,
          "procedural terrain can be queried far from origin");
    check(first.blockAt(2, 100, 2) == GC_BLOCK_AIR,
          "air exists above the terrain surface");
    (void)otherSeed;

    glitchcraft::Game sharedWorld(1234U);
    check(sharedWorld.applyBlockEdit(20, 8, -4, GC_BLOCK_WOOD) &&
              sharedWorld.blockAt(20, 8, -4) == GC_BLOCK_WOOD,
          "a remote block edit updates the shared world");
    check(!sharedWorld.applyBlockEdit(20, 8, -4,
                                     static_cast<GcBlock>(GC_BLOCK_COUNT)),
          "invalid remote block edits are rejected");
    sharedWorld.setRemotePlayer(2, 4.0F, 6.0F, 8.0F, 1.0F, 0.1F);
    check(sharedWorld.remotePlayers().size() == 1U &&
              sharedWorld.remotePlayers()[0].id == 2,
          "remote player state is available to renderers");
    sharedWorld.setRemotePlayer(2, 5.0F, 6.0F, 8.0F, 1.0F, 0.1F);
    check(sharedWorld.remotePlayers().size() == 1U &&
              sharedWorld.remotePlayers()[0].x == 5.0F,
          "remote player updates replace stale position");
    sharedWorld.removeRemotePlayer(2);
    check(sharedWorld.remotePlayers().empty(),
          "disconnected remote players are removed");

    std::string realmsError;
    auto hostSession = glitchcraft::LanSession::host(98765U, realmsError, 0);
    check(hostSession != nullptr, "a private LAN world can be hosted");
    if (hostSession != nullptr) {
        glitchcraft::Game hostWorld(hostSession->worldSeed());
        check(hostWorld.applyBlockEdit(24, 9, -7, GC_BLOCK_STONE),
              "host world can stage a block for joining players");
        std::atomic<bool> stopHost{false};
        std::thread hostPump([&]() {
            while (!stopHost.load()) {
                hostSession->pump(hostWorld);
                hostSession->publishPosition(hostWorld);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        std::ostringstream address;
        address << "127.0.0.1:" << hostSession->port();
        auto clientSession =
            glitchcraft::LanSession::join(address.str(), realmsError);
        check(clientSession != nullptr,
              "a client can join a local hosted world");
        if (clientSession != nullptr) {
            check(clientSession->worldSeed() == hostSession->worldSeed(),
                  "joining client receives the host world seed");
            glitchcraft::Game clientWorld(clientSession->worldSeed());
            bool editSent = false;
            for (int attempt = 0; attempt < 100; ++attempt) {
                if (!editSent &&
                    clientWorld.applyBlockEdit(31, 11, 13, GC_BLOCK_WOOD)) {
                    clientSession->publishBlock(31, 11, 13, GC_BLOCK_WOOD);
                    editSent = true;
                }
                clientSession->publishPosition(clientWorld);
                clientSession->pump(clientWorld);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            check(clientWorld.blockAt(24, 9, -7) == GC_BLOCK_STONE,
                  "joining client receives the existing world snapshot");
            check(!clientWorld.remotePlayers().empty() &&
                      clientWorld.remotePlayers()[0].id == 1,
                  "joining client sees the host player");
            check(clientSession->error().empty(),
                  "LAN client remains connected during synchronization");
        }
        stopHost.store(true);
        hostPump.join();
        if (clientSession != nullptr) {
            check(hostWorld.blockAt(31, 11, 13) == GC_BLOCK_WOOD,
                  "host receives block edits from joined clients");
            check(!hostWorld.remotePlayers().empty(),
                  "host receives joined player positions");
        }
    } else {
        std::cerr << "LAN host setup failed: " << realmsError << '\n';
    }

    for (int block = GC_BLOCK_GRASS; block < GC_BLOCK_COUNT; ++block) {
        const auto texture = glitchcraft::gameengine::blockTexture(
            static_cast<GcBlock>(block), 0, 1, 0);
        const auto firstTexel =
            glitchcraft::gameengine::sampleTexture(texture, 0, 0);
        bool hasTextureDetail = false;
        for (int v = 0; v < 16 && !hasTextureDetail; ++v) {
            for (int u = 0; u < 16; ++u) {
                const auto texel =
                    glitchcraft::gameengine::sampleTexture(texture, u, v);
                if (texel.red != firstTexel.red ||
                    texel.green != firstTexel.green ||
                    texel.blue != firstTexel.blue) {
                    hasTextureDetail = true;
                    break;
                }
                glitchcraft::Game shulkerWorld(1234U);
                for (int color = GC_BLOCK_SHULKER_WHITE;
                     color <= GC_BLOCK_SHULKER_BLACK; ++color) {
                    const GcBlock box = static_cast<GcBlock>(color);
                    const int x = color * 3;
                    check(shulkerWorld.applyBlockEdit(x, 20, 30, box) &&
                              shulkerWorld.isShulkerBox(x, 20, 30),
                          "each colored shulker box can be placed in the world");
                    glitchcraft::ShulkerStack slot;
                    check(shulkerWorld.getShulkerSlot(x, 20, 30, 0, slot) &&
                              slot.block == GC_BLOCK_AIR && slot.count == 0,
                          "new shulker boxes have 27 empty storage slots");
                    check(shulkerWorld.setShulkerSlot(x, 20, 30, 0, GC_BLOCK_STONE, 64) &&
                              shulkerWorld.getShulkerSlot(x, 20, 30, 0, slot) &&
                              slot.block == GC_BLOCK_STONE && slot.count == 64,
                          "shulker boxes store a stack of blocks");
                    check(!shulkerWorld.getShulkerSlot(x, 20, 30, 27, slot) &&
                              !shulkerWorld.setShulkerSlot(
                                  x, 20, 30, 0, GC_BLOCK_STONE, 65),
                          "shulker storage rejects invalid slots and stack sizes");
                    const auto texture = glitchcraft::gameengine::blockTexture(
                        box, 0, 1, 0);
                    check(glitchcraft::gameengine::sampleTexture(texture, 8, 8).red !=
                              glitchcraft::gameengine::sampleTexture(texture, 0, 0).red,
                          "each shulker color uses a textured box material");
                    check(shulkerWorld.applyBlockEdit(x, 20, 30, GC_BLOCK_AIR) &&
                              !shulkerWorld.isShulkerBox(x, 20, 30) &&
                              !shulkerWorld.getShulkerSlot(x, 20, 30, 0, slot),
                          "removing a shulker box removes its contents");
                }
            }
        }
        check(hasTextureDetail,
              "each visible block material has a detailed texture");
    }
    for (const auto texture : {glitchcraft::gameengine::TextureId::Rail,
                               glitchcraft::gameengine::TextureId::Tie}) {
        check(glitchcraft::gameengine::sampleTexture(texture, 0, 0).red !=
                  glitchcraft::gameengine::sampleTexture(texture, 4, 7).red,
              "rail and sleeper materials have texture detail");
    }

    const auto target = first.castRay(first.yaw(), first.pitch(), 12.0F);
    check(target.hit, "first-person ray reaches generated terrain");
    if (target.hit) {
        check(first.placeBlock(GC_BLOCK_WOOD), "a block can be placed adjacent to a hit");
        check(first.blockAt(target.adjacent.x, target.adjacent.y,
                            target.adjacent.z) == GC_BLOCK_WOOD,
              "placed block is stored in the edited world");
        check(first.mineBlock(), "a targeted block can be mined");
        check(first.blockAt(target.adjacent.x, target.adjacent.y,
                            target.adjacent.z) == GC_BLOCK_AIR,
              "mining removes the targeted block");
    }

    const float x = first.playerX();
    const float z = first.playerZ();
    check(first.move(1.0F, 0.0F), "player can move on open terrain");
    check(first.playerX() != x || first.playerZ() != z,
          "movement updates player position");
    glitchcraft::Game normalSpeed(1234U);
    glitchcraft::Game sprintSpeed(1234U);
    normalSpeed.move(1.0F, 0.0F);
    sprintSpeed.move(1.0F, 0.0F, true);
    check(std::abs(sprintSpeed.playerX() - 0.5F) >
              std::abs(normalSpeed.playerX() - 0.5F),
          "sprint increases movement speed");

    first.drinkPotion();
    check(first.potionTicks() > 0, "potion applies a timed effect");
    first.drinkMilk();
    check(first.potionTicks() == 0, "milk clears potion effects");
    const int foodBefore = first.foodLevel();
    check(first.eatFood(GC_FOOD_CAKE), "food can be eaten");
    check(first.foodLevel() > foodBefore, "food replenishes the food meter");
    check(!first.eatFood(static_cast<GcFood>(GC_FOOD_COUNT)),
          "invalid food types are rejected");
    const int fedLevel = first.foodLevel();
    for (int tick = 0; tick < 1800; ++tick) {
        first.tick();
    }
    check(first.foodLevel() == fedLevel - 1,
          "food meter depletes gradually during play");
    first.jump();
    first.tick();
    check(first.playerY() > static_cast<float>(glitchcraft::terrainHeight(0, 0, 1234U)),
          "jump applies upward movement");
    first.setFlying(true);
    check(first.flying(), "flight can be enabled");
    first.setPaused(true);
    check(first.paused(), "pause state is recorded");

    glitchcraft::Game railGame(1234U);
    glitchcraft::Game vehicleWithoutRail(1234U);
    check(!vehicleWithoutRail.placeVehicle(GC_VEHICLE_BOXCAR),
          "rolling stock cannot be placed before rails");
    check(railGame.placeTrack(GC_TRACK_STRAIGHT, true),
          "track can be placed above a solid terrain block");
    const auto railHit = railGame.castRay(
        railGame.yaw(), railGame.pitch(), 12.0F);
    check(railHit.hit && railHit.track,
          "placed track can be targeted for vehicle placement");
    check(railGame.placeVehicle(GC_VEHICLE_CSX_DIESEL),
          "diesel locomotive can be placed on a rail");
    check(railGame.vehicles().size() == 1U &&
              railGame.vehicles()[0].fuelType == GC_TRAIN_FUEL_DIESEL,
          "diesel locomotive receives diesel fuel");
    check(railGame.addTrainFuel(250),
          "targeted diesel locomotive accepts compatible fuel");
    check(!railGame.vehicles().empty() && railGame.vehicles()[0].fuel == 350,
          "refueling increases locomotive fuel");
    check(!railGame.loadTrainFuel(GC_TRAIN_FUEL_OIL, 100),
          "diesel locomotives reject oil");
    check(railGame.loadTrainFuel(GC_TRAIN_FUEL_DIESEL, 10000) &&
              railGame.vehicles()[0].fuel == 1000,
          "fuel loading clamps safely to the 1000-unit tank capacity");
    check(!railGame.loadTrainFuel(GC_TRAIN_FUEL_DIESEL, 1),
          "full tanks reject additional fuel");
    check(!railGame.loadTrainFuel(GC_TRAIN_FUEL_DIESEL, -1),
          "fuel loading rejects non-positive quantities");
    railGame.toggleTrainPower();
    check(!railGame.vehicles().empty() && railGame.vehicles()[0].powered,
          "train power can be toggled on");
    check(!railGame.mineBlock(),
          "occupied rail cannot be mined");
    check(!railGame.removeTrackAt(railHit.position.x, railHit.position.y,
                                  railHit.position.z),
          "track API cannot remove rail under rolling stock");

    for (int type = GC_TRACK_STRAIGHT; type < GC_TRACK_COUNT; ++type) {
        glitchcraft::Game trackTypeGame(1234U);
        check(trackTypeGame.placeTrack(static_cast<GcTrackType>(type), true),
              "every track type can be placed on supported terrain");
    }
    for (int type = GC_VEHICLE_BIG_BOY; type < GC_VEHICLE_COUNT; ++type) {
        glitchcraft::Game vehicleTypeGame(1234U);
        const bool placed =
            vehicleTypeGame.placeTrack(GC_TRACK_STRAIGHT, true) &&
            vehicleTypeGame.placeVehicle(static_cast<GcVehicleType>(type));
        check(placed,
              "every locomotive, tender, coach, and freight car can be placed on rail");
        if (placed && type == GC_VEHICLE_BIG_BOY_TENDER) {
            check(vehicleTypeGame.vehicles()[0].fuelType == GC_TRAIN_FUEL_OIL &&
                      vehicleTypeGame.vehicles()[0].fuel == 100,
                  "Big Boy tender starts with oil");
        } else if (placed && type == GC_VEHICLE_FLYING_SCOTSMAN_TENDER) {
            check(vehicleTypeGame.vehicles()[0].fuelType == GC_TRAIN_FUEL_OIL &&
                      vehicleTypeGame.vehicles()[0].fuel == 100,
                  "Flying Scotsman tender starts with oil");
        } else if (placed && type == GC_VEHICLE_BIG_BOY) {
            check(vehicleTypeGame.vehicles()[0].fuel == 0,
                  "Big Boy requires tender-supplied oil");
        } else if (placed && type == GC_VEHICLE_FLYING_SCOTSMAN) {
            check(vehicleTypeGame.vehicles()[0].fuelType == GC_TRAIN_FUEL_OIL &&
                      vehicleTypeGame.vehicles()[0].fuel == 0,
                  "Flying Scotsman requires tender-supplied oil");
        }
    }
    check(glitchcraft::gameengine::isLocomotive(GC_VEHICLE_FLYING_SCOTSMAN) &&
              glitchcraft::gameengine::isSteamTender(
                  GC_VEHICLE_FLYING_SCOTSMAN_TENDER) &&
              glitchcraft::gameengine::isPassengerVehicle(
                  GC_VEHICLE_FLYING_SCOTSMAN_COACH) &&
              glitchcraft::gameengine::steamTenderFor(
                  GC_VEHICLE_FLYING_SCOTSMAN) ==
                  GC_VEHICLE_FLYING_SCOTSMAN_TENDER &&
              glitchcraft::gameengine::vehicleName(
                  GC_VEHICLE_FLYING_SCOTSMAN_COACH).find("Coach") !=
                  std::string::npos,
          "Flying Scotsman locomotive, tender, and separate coach are catalogued");
    for (int index = 0; index < 9; ++index) {
        const auto colorEngine = static_cast<GcVehicleType>(
            GC_VEHICLE_SHINKANSEN_ENGINE_RED + index);
        const auto rearEngine = static_cast<GcVehicleType>(
            GC_VEHICLE_SHINKANSEN_REAR_ENGINE_RED + index);
        const auto coach = static_cast<GcVehicleType>(
            GC_VEHICLE_SHINKANSEN_COACH_RED + index);
        check(glitchcraft::gameengine::isLocomotive(colorEngine) &&
                  glitchcraft::gameengine::isLocomotive(rearEngine) &&
                  glitchcraft::gameengine::isPassengerVehicle(coach),
              "each Shinkansen color has front/rear engines and a separate coach");
        check(glitchcraft::gameengine::vehicleColor(colorEngine).red ==
                  glitchcraft::gameengine::vehicleColor(coach).red &&
                  glitchcraft::gameengine::vehicleColor(rearEngine).green ==
                      glitchcraft::gameengine::vehicleColor(coach).green,
              "Shinkansen engines and coaches share matching color liveries");
        check(glitchcraft::gameengine::isElectricLocomotive(colorEngine) &&
                  glitchcraft::gameengine::vehicleName(coach).find("Coach") !=
                      std::string::npos,
              "Shinkansen locomotives are electric and coaches are named");
    }
    for (int index = 0; index < 26; ++index) {
        const auto engineType = static_cast<GcVehicleType>(
            GC_VEHICLE_SUBWAY_ENGINE_A + index);
        const auto coachType = static_cast<GcVehicleType>(
            GC_VEHICLE_SUBWAY_COACH_A + index);
        const char routeLetter = static_cast<char>('A' + index);
        check(glitchcraft::gameengine::isElectricLocomotive(engineType) &&
                  glitchcraft::gameengine::isPassengerVehicle(coachType),
              "every A-Z subway route has an electric engine and separate coach");
        check(glitchcraft::gameengine::vehicleName(engineType).find(routeLetter) !=
                      std::string::npos &&
                  glitchcraft::gameengine::vehicleName(coachType).find(routeLetter) !=
                      std::string::npos,
              "subway engine and coach names preserve their route letter");
        check(glitchcraft::gameengine::vehicleColor(engineType).red ==
                  glitchcraft::gameengine::vehicleColor(coachType).red &&
                  glitchcraft::gameengine::vehicleColor(engineType).blue ==
                      glitchcraft::gameengine::vehicleColor(coachType).blue,
              "subway engine and coach use matching route colors");
    }
    for (const auto type : {GC_VEHICLE_SHINKANSEN_ENGINE_RED,
                            GC_VEHICLE_SUBWAY_ENGINE_A}) {
        glitchcraft::Game electricTrain(1234U);
        const bool placed = electricTrain.placeTrack(GC_TRACK_STRAIGHT, true) &&
                            electricTrain.placeVehicle(type);
        check(placed &&
                  electricTrain.vehicles()[0].fuelType == GC_TRAIN_FUEL_NONE &&
                  electricTrain.vehicles()[0].fuel == 0,
              "electric passenger train engines do not require onboard fuel");
        if (placed) {
            electricTrain.setTrainPower(true);
            check(electricTrain.vehicles()[0].powered,
                  "electric passenger train engines can be powered");
            check(!electricTrain.addTrainFuel(100),
                  "electric passenger engines reject diesel and oil refueling");
        }
    }
    glitchcraft::Game switchGame(1234U);
    check(switchGame.placeTrack(GC_TRACK_SWITCH, true),
          "switch rail can be placed");
    switchGame.toggleTrackSwitch();
    const auto switchTrack = switchGame.tracks().begin();
    check(switchTrack != switchGame.tracks().end() &&
              switchTrack->second.switchBranch,
          "targeted switch changes branch state");
    const auto switchPosition = switchTrack->second.position;
    glitchcraft::Track queriedTrack{};
    check(switchGame.getTrackAt(switchPosition.x, switchPosition.y,
                                switchPosition.z, queriedTrack) &&
              queriedTrack.type == GC_TRACK_SWITCH &&
              queriedTrack.switchBranch,
          "track API queries track state at exact coordinates");
    check(switchGame.setTrackSwitch(switchPosition.x, switchPosition.y,
                                    switchPosition.z, false) &&
              switchGame.getTrackAt(switchPosition.x, switchPosition.y,
                                    switchPosition.z, queriedTrack) &&
              !queriedTrack.switchBranch,
          "track API sets a switch branch explicitly");
    glitchcraft::Game removableTrackGame(1234U);
    check(removableTrackGame.placeTrack(GC_TRACK_STRAIGHT, true) &&
              removableTrackGame.mineBlock() &&
              removableTrackGame.tracks().empty(),
          "unoccupied rail can be mined and removed");
    check(!switchGame.placeTrack(static_cast<GcTrackType>(GC_TRACK_COUNT), true),
          "invalid track types are rejected");
    check(!switchGame.placeVehicle(
              static_cast<GcVehicleType>(GC_VEHICLE_COUNT)),
          "invalid vehicle types are rejected");
    glitchcraft::Game directTrackGame(1234U);
    const int directTrackY = glitchcraft::terrainHeight(30, 30, 1234U);
    check(directTrackGame.setTrackAt(30, directTrackY, 30,
                                     GC_TRACK_CURVED, false) &&
              directTrackGame.getTrackAt(30, directTrackY, 30, queriedTrack) &&
              queriedTrack.type == GC_TRACK_CURVED &&
              !queriedTrack.axisX,
          "track API creates and reads rails at world coordinates");
    check(directTrackGame.removeTrackAt(30, directTrackY, 30) &&
              !directTrackGame.getTrackAt(30, directTrackY, 30, queriedTrack),
          "track API removes an unoccupied rail by coordinates");

    const GcBlock roadBlocks[]{
        GC_BLOCK_ASPHALT, GC_BLOCK_ROAD_STRAIGHT, GC_BLOCK_ROAD_CURVE,
        GC_BLOCK_ROAD_T_INTERSECTION, GC_BLOCK_ROAD_CROSS_INTERSECTION,
        GC_BLOCK_ROAD_DEAD_END, GC_BLOCK_ROAD_ROUNDABOUT};
    const glitchcraft::gameengine::TextureId roadTextures[]{
        glitchcraft::gameengine::TextureId::Asphalt,
        glitchcraft::gameengine::TextureId::RoadStraight,
        glitchcraft::gameengine::TextureId::RoadCurve,
        glitchcraft::gameengine::TextureId::RoadTIntersection,
        glitchcraft::gameengine::TextureId::RoadCrossIntersection,
        glitchcraft::gameengine::TextureId::RoadDeadEnd,
        glitchcraft::gameengine::TextureId::RoadRoundabout};
    glitchcraft::Game roadGame(1234U);
    for (std::size_t index = 0; index < std::size(roadBlocks); ++index) {
        const int x = 100 + static_cast<int>(index);
        check(roadGame.applyBlockEdit(x, 20, 20, roadBlocks[index]) &&
                  roadGame.blockAt(x, 20, 20) == roadBlocks[index],
              "asphalt and every road layout are placeable blocks");
        check(glitchcraft::gameengine::blockTexture(
                  roadBlocks[index], 0, 1, 0) == roadTextures[index],
              "each road layout selects its own top-surface texture");
        if (index > 0) {
            check(glitchcraft::gameengine::blockTexture(
                      roadBlocks[index], 0, 0, 1) ==
                      glitchcraft::gameengine::TextureId::Asphalt,
                  "road block sides use the asphalt material");
        }
    }

    const float lhs[4]{1.0F, 2.0F, 3.0F, 4.0F};
    const float rhs[4]{5.0F, 6.0F, 7.0F, 8.0F};
    check(std::abs(gc_voxel_dot4(lhs, rhs) - 70.0F) < 0.001F,
          "portable or PowerPC vector dot product is correct");
    float depth[3]{1.0F, 2.0F, 3.0F};
    gc_engine_clear_depth(depth, 3, 0.0F);
    check(depth[0] == 0.0F && depth[1] == 0.0F && depth[2] == 0.0F,
          "portable or PowerPC depth buffer kernel clears every entry");
    glitchcraft::gameengine::Renderer renderer(96, 64);
    renderer.render(first);
    check(renderer.pixels().size() == 96U * 64U,
          "3D renderer produces a complete pixel frame");
    int rasterizedPixels = 0;
    const float skyBlend = std::clamp(
        0.5F + std::sin(first.pitch()) * 0.4F, 0.0F, 1.0F);
    for (int y = 0; y < renderer.height(); ++y) {
        for (int x = 0; x < renderer.width(); ++x) {
            const float vertical = static_cast<float>(y) /
                                   static_cast<float>(renderer.height());
            const float horizontal = static_cast<float>(x) /
                                    static_cast<float>(renderer.width());
            const bool sky = vertical < 0.47F - skyBlend * 0.24F;
            const int fade = static_cast<int>(
                12.0F * std::sin(horizontal * pi) *
                std::sin(vertical * pi));
            const std::uint32_t background = sky
                ? (static_cast<std::uint32_t>(104 + fade) << 16U) |
                      (static_cast<std::uint32_t>(173 + fade) << 8U) |
                      static_cast<std::uint32_t>(233 + fade)
                : (static_cast<std::uint32_t>(102 + fade) << 16U) |
                      (static_cast<std::uint32_t>(132 + fade) << 8U) |
                      static_cast<std::uint32_t>(74 + fade);
            const auto index = static_cast<std::size_t>(y) *
                                   static_cast<std::size_t>(renderer.width()) +
                               static_cast<std::size_t>(x);
            if (renderer.pixels()[index] != background) {
                ++rasterizedPixels;
            }
        }
    }
    check(rasterizedPixels > 100, "3D renderer rasterizes voxel geometry");
    glitchcraft::Game plainRailWorld(1234U);
    glitchcraft::gameengine::Renderer railRenderer(96, 64);
    glitchcraft::gameengine::Renderer plainRailRenderer(96, 64);
    railRenderer.render(railGame);
    plainRailRenderer.render(plainRailWorld);
    check(railRenderer.pixels() != plainRailRenderer.pixels(),
          "desktop 3D renderer draws rails and rolling stock");

    GcGame *apiGame = gc_game_create(42U);
    check(apiGame != nullptr, "C API creates a game");
    check(gc_game_get_block(apiGame, 0, 0, 0) == GC_BLOCK_STONE,
          "C API exposes generated world blocks");
    gc_game_drink_potion(apiGame);
    check(gc_game_eat_food(apiGame, GC_FOOD_COOKIE) == 1,
          "C API exposes food consumption");
    check(gc_game_apply_block_edit(apiGame, 20, 30, 40,
                                   GC_BLOCK_SHULKER_PURPLE) == 1,
          "C API can apply a colored shulker block edit");
    check(gc_game_set_shulker_slot(apiGame, 20, 30, 40, 26,
                                   GC_BLOCK_REDSTONE, 16) == 1,
          "C API stores a stack in the last shulker slot");
    GcShulkerStack cStack{};
    check(gc_game_get_shulker_slot(apiGame, 20, 30, 40, 26, &cStack) == 1 &&
              cStack.block == GC_BLOCK_REDSTONE && cStack.count == 16,
          "C API reads shulker slot contents");
    GcPlayerState state{};
    gc_game_get_player(apiGame, &state);
    check(state.potion_ticks > 0, "C API exposes potion state");
    check(gc_game_place_track(apiGame, GC_TRACK_STRAIGHT, 1) == 1,
          "C API supports rail placement");
    check(gc_game_place_vehicle(apiGame, GC_VEHICLE_BNSF_DIESEL) == 1,
          "C API supports rolling-stock placement");
    check(gc_game_track_count(apiGame) == 1 &&
              gc_game_vehicle_count(apiGame) == 1,
          "C API reports placed rail and vehicle counts");
    GcTrackState trackState{};
    check(gc_game_get_track(apiGame, 0, &trackState) == 1 &&
              trackState.type == GC_TRACK_STRAIGHT && trackState.axis_x == 1 &&
              gc_game_get_track_at(apiGame, trackState.x, trackState.y,
                                   trackState.z, &trackState) == 1,
          "C API exposes rail type and orientation");
    GcVehicleState vehicleState{};
    check(gc_game_get_vehicle(apiGame, 0, &vehicleState) == 1 &&
              vehicleState.type == GC_VEHICLE_BNSF_DIESEL &&
              vehicleState.fuel_type == GC_TRAIN_FUEL_DIESEL,
          "C API exposes locomotive fuel and type state");
    check(gc_game_add_train_fuel(apiGame, 75) == 1,
          "C API supports compatible train refueling");
    check(gc_game_load_train_fuel(apiGame, GC_TRAIN_FUEL_DIESEL, 75) == 1 &&
              gc_game_load_train_fuel(apiGame, GC_TRAIN_FUEL_OIL, 75) == 0,
          "C API accepts only the locomotive's compatible fuel type");
    gc_game_toggle_train_power(apiGame);
    check(gc_game_get_vehicle(apiGame, 0, &vehicleState) == 1 &&
              vehicleState.powered == 1,
          "C API toggles locomotive power");
    gc_game_tick(apiGame);
    gc_game_destroy(apiGame);

    GcGame *trackApiGame = gc_game_create(42U);
    check(trackApiGame != nullptr, "C API creates a track-edit test world");
    const int apiTrackY = glitchcraft::terrainHeight(30, 30, 42U);
    check(gc_game_set_track_at(trackApiGame, 30, apiTrackY, 30,
                               GC_TRACK_SWITCH, 1) == 1,
          "C API can place a track at explicit world coordinates");
    check(gc_game_set_track_switch(trackApiGame, 30, apiTrackY, 30, 1) == 1 &&
              gc_game_get_track_at(trackApiGame, 30, apiTrackY, 30,
                                   &trackState) == 1 &&
              trackState.type == GC_TRACK_SWITCH &&
              trackState.switch_branch == 1,
          "C API can query and set an explicit switch branch");
    check(gc_game_remove_track_at(trackApiGame, 30, apiTrackY, 30) == 1,
          "C API removes unoccupied track by world coordinate");
    gc_game_destroy(trackApiGame);

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All engine tests passed\n";
    return EXIT_SUCCESS;
}
