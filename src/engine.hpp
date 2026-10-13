#ifndef GLITCHCRAFT_ENGINE_HPP
#define GLITCHCRAFT_ENGINE_HPP

#include "glitchcraft/game_api.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace glitchcraft {

struct Position {
    int x;
    int y;
    int z;

    bool operator==(const Position &other) const noexcept;
};

struct PositionHash {
    std::size_t operator()(const Position &position) const noexcept;
};

struct RayHit {
    GcBlock block = GC_BLOCK_AIR;
    Position position{0, 0, 0};
    Position adjacent{0, 0, 0};
    float distance = 0.0F;
    float side = 0.0F;
    bool hit = false;
    bool track = false;
};

struct Track {
    Position position;
    GcTrackType type;
    bool axisX;
    bool switchBranch;
};

struct TrainVehicle {
    GcVehicleType type;
    Position trackPosition;
    bool facingX;
    int direction;
    int consist;
    int consistOrder;
    int fuelCounter;
    GcTrainFuel fuelType;
    int fuel;
    bool powered;
};

struct RemotePlayer {
    int id;
    float x;
    float y;
    float z;
    float yaw;
    float pitch;
};

int terrainHeight(int x, int z, std::uint32_t seed);

class Game {
public:
    explicit Game(std::uint32_t seed);

    GcBlock blockAt(int x, int y, int z) const;
    RayHit castRay(float yaw, float pitch, float maxDistance) const;
    bool move(float forward, float strafe, bool sprint = false);
    void jump();
    void look(float yawDelta, float pitchDelta);
    bool placeBlock(GcBlock block);
    bool applyBlockEdit(int x, int y, int z, GcBlock block);
    bool placeTrack(GcTrackType type, bool axisX = true);
    bool placeVehicle(GcVehicleType type);
    bool mineBlock();
    void drinkPotion();
    void drinkMilk();
    bool eatFood(GcFood food);
    void setFlying(bool enabled);
    void setPaused(bool paused);
    void tick();

    float playerX() const noexcept;
    float playerY() const noexcept;
    float playerZ() const noexcept;
    float yaw() const noexcept;
    float pitch() const noexcept;
    int potionTicks() const noexcept;
    int foodLevel() const noexcept;
    bool flying() const noexcept;
    bool paused() const noexcept;
    const std::unordered_map<Position, Track, PositionHash> &tracks() const noexcept;
    const std::vector<TrainVehicle> &vehicles() const noexcept;
    const std::unordered_map<Position, GcBlock, PositionHash> &
    blockEdits() const noexcept;
    const std::vector<RemotePlayer> &remotePlayers() const noexcept;
    void setRemotePlayer(int id, float x, float y, float z, float yaw,
                         float pitch);
    void removeRemotePlayer(int id);
    bool addTrainFuel(int amount);
    void setTrainPower(bool enabled);
    void toggleTrainPower();
    void toggleTrackSwitch();

private:
    std::uint32_t seed_;
    float x_ = 0.5F;
    float y_ = 2.0F;
    float z_ = 0.5F;
    float yaw_ = 0.0F;
    float pitch_ = -0.35F;
    int potionTicks_ = 0;
    int foodLevel_ = 80;
    int foodTickCounter_ = 0;
    float verticalVelocity_ = 0.0F;
    bool flying_ = false;
    bool paused_ = false;
    std::unordered_map<Position, GcBlock, PositionHash> edits_;
    std::unordered_map<Position, Track, PositionHash> tracks_;
    std::vector<TrainVehicle> vehicles_;
    std::vector<RemotePlayer> remotePlayers_;
    int nextConsistId_ = 1;

    GcBlock generatedBlock(int x, int y, int z) const;
    bool collides(float x, float y, float z) const;
    void changeView(float yawDelta, float pitchDelta);
    bool trackSupportsVehicles(const Track &track) const;
    void advanceTrains();
};

}

#endif
