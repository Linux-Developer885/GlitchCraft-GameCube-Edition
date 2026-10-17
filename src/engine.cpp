#include "engine.hpp"
#include "GameEngine/vehicles.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace glitchcraft {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kPlayerRadius = 0.22F;
constexpr float kPlayerHeight = 1.72F;

int floorToInt(float value)
{
    return static_cast<int>(std::floor(value));
}

std::uint32_t coordinateHash(int x, int z, std::uint32_t seed)
{
    std::uint32_t value = static_cast<std::uint32_t>(x) * 0x8da6b343U;
    value ^= static_cast<std::uint32_t>(z) * 0xd8163841U;
    value ^= seed * 0xcb1ab31fU;
    value ^= value >> 13U;
    value *= 0x85ebca6bU;
    value ^= value >> 16U;
    return value;
}

}

bool Position::operator==(const Position &other) const noexcept
{
    return x == other.x && y == other.y && z == other.z;
}

std::size_t PositionHash::operator()(const Position &position) const noexcept
{
    const auto hash = coordinateHash(position.x, position.z,
                                     static_cast<std::uint32_t>(position.y));
    return static_cast<std::size_t>(hash);
}

Game::Game(std::uint32_t seed)
    : seed_(seed)
{
    y_ = static_cast<float>(terrainHeight(0, 0, seed_)) + 0.001F;
}

int terrainHeight(int x, int z, std::uint32_t seed)
{
    const int broadValue = (x / 11 + z / 13) % 3;
    const int broad = (broadValue + 3) % 3;
    const int detail = static_cast<int>(coordinateHash(x, z, seed) % 3U);
    return 4 + broad + detail;
}

GcBlock Game::generatedBlock(int x, int y, int z) const
{
    if (y < 0) {
        return GC_BLOCK_STONE;
    }

    const int height = terrainHeight(x, z, seed_);
    if (y < height - 3) {
        return GC_BLOCK_STONE;
    }
    if (y < height - 1) {
        return GC_BLOCK_DIRT;
    }
    if (y == height - 1) {
        const auto feature = coordinateHash(x, z, seed_) % 97U;
        if (feature == 0U) {
            return GC_BLOCK_WATER;
        }
        if (feature == 1U) {
            return GC_BLOCK_LAVA;
        }
        return GC_BLOCK_GRASS;
    }
    return GC_BLOCK_AIR;
}

GcBlock Game::blockAt(int x, int y, int z) const
{
    const auto edit = edits_.find(Position{x, y, z});
    return edit == edits_.end() ? generatedBlock(x, y, z) : edit->second;
}

bool Game::isShulkerBox(int x, int y, int z) const
{
    const GcBlock block = blockAt(x, y, z);
    return block >= GC_BLOCK_SHULKER_WHITE &&
           block <= GC_BLOCK_SHULKER_BLACK;
}

bool Game::getShulkerSlot(int x, int y, int z, int slot,
                          ShulkerStack &stack) const
{
    if (slot < 0 || slot >= 27 || !isShulkerBox(x, y, z)) {
        return false;
    }
    const auto contents = shulkerContents_.find(Position{x, y, z});
    if (contents == shulkerContents_.end()) {
        stack = ShulkerStack{};
    } else {
        stack = contents->second[static_cast<std::size_t>(slot)];
    }
    return true;
}

bool Game::setShulkerSlot(int x, int y, int z, int slot, GcBlock block,
                          int count)
{
    if (slot < 0 || slot >= 27 || !isShulkerBox(x, y, z) ||
        count < 0 || count > 64 ||
        (count > 0 && (block <= GC_BLOCK_AIR || block >= GC_BLOCK_COUNT))) {
        return false;
    }
    auto &contents = shulkerContents_[Position{x, y, z}];
    contents[static_cast<std::size_t>(slot)] =
        count == 0 ? ShulkerStack{} : ShulkerStack{block, count};
    return true;
}

void Game::syncShulkerBox(Position position, GcBlock block)
{
    if (block >= GC_BLOCK_SHULKER_WHITE &&
        block <= GC_BLOCK_SHULKER_BLACK) {
        shulkerContents_.try_emplace(position);
    } else {
        shulkerContents_.erase(position);
    }
}

bool Game::trackSupportsVehicles(const Track &track) const
{
    if (blockAt(track.position.x, track.position.y, track.position.z) !=
        GC_BLOCK_AIR) {
        return false;
    }
    const GcBlock support = blockAt(track.position.x, track.position.y - 1,
                                    track.position.z);
    return support != GC_BLOCK_AIR && support != GC_BLOCK_WATER &&
           support != GC_BLOCK_LAVA;
}

bool Game::collides(float x, float y, float z) const
{
    const int minX = floorToInt(x - kPlayerRadius);
    const int maxX = floorToInt(x + kPlayerRadius);
    const int minY = floorToInt(y);
    const int maxY = floorToInt(y + kPlayerHeight);
    const int minZ = floorToInt(z - kPlayerRadius);
    const int maxZ = floorToInt(z + kPlayerRadius);

    for (int blockX = minX; blockX <= maxX; ++blockX) {
        for (int blockY = minY; blockY <= maxY; ++blockY) {
            for (int blockZ = minZ; blockZ <= maxZ; ++blockZ) {
                const GcBlock block = blockAt(blockX, blockY, blockZ);
                if (block != GC_BLOCK_AIR && block != GC_BLOCK_WATER &&
                    tracks_.find(Position{blockX, blockY, blockZ}) ==
                        tracks_.end()) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Game::move(float forward, float strafe, bool sprint)
{
    constexpr float kStep = 0.18F;
    const float length = std::sqrt(forward * forward + strafe * strafe);
    if (length > 1.0F) {
        forward /= length;
        strafe /= length;
    }
    const float speed = (potionTicks_ > 0 ? 1.5F : 1.0F) *
                        (sprint ? 2.0F : 1.0F);
    const float moveX = (std::cos(yaw_) * forward -
                         std::sin(yaw_) * strafe) * kStep * speed;
    const float moveZ = (std::sin(yaw_) * forward +
                         std::cos(yaw_) * strafe) * kStep * speed;
    const float nextX = x_ + moveX;
    const float nextZ = z_ + moveZ;

    bool moved = false;
    if (flying_) {
        x_ = nextX;
        z_ = nextZ;
        moved = true;
    } else {
        if (!collides(nextX, y_, z_)) {
            x_ = nextX;
            moved = moved || moveX != 0.0F;
        }
        if (!collides(x_, y_, nextZ)) {
            z_ = nextZ;
            moved = moved || moveZ != 0.0F;
        }
    }
    return moved;
}

RayHit Game::castRay(float yaw, float pitch, float maxDistance) const
{
    const float horizontalScale = std::cos(pitch);
    const float dirX = std::cos(yaw) * horizontalScale;
    const float dirY = std::sin(pitch);
    const float dirZ = std::sin(yaw) * horizontalScale;
    int mapX = floorToInt(x_);
    int mapY = floorToInt(y_ + 1.45F);
    int mapZ = floorToInt(z_);
    const float deltaX = dirX == 0.0F ? std::numeric_limits<float>::max()
                                      : std::abs(1.0F / dirX);
    const float deltaY = dirY == 0.0F ? std::numeric_limits<float>::max()
                                      : std::abs(1.0F / dirY);
    const float deltaZ = dirZ == 0.0F ? std::numeric_limits<float>::max()
                                      : std::abs(1.0F / dirZ);
    const int stepX = dirX < 0.0F ? -1 : 1;
    const int stepY = dirY < 0.0F ? -1 : 1;
    const int stepZ = dirZ < 0.0F ? -1 : 1;
    float sideX = dirX < 0.0F ? (x_ - static_cast<float>(mapX)) * deltaX
                              : (static_cast<float>(mapX + 1) - x_) * deltaX;
    const float rayY = y_ + 1.45F;
    float sideY = dirY < 0.0F ? (rayY - static_cast<float>(mapY)) * deltaY
                              : (static_cast<float>(mapY + 1) - rayY) * deltaY;
    float sideZ = dirZ < 0.0F ? (z_ - static_cast<float>(mapZ)) * deltaZ
                              : (static_cast<float>(mapZ + 1) - z_) * deltaZ;
    int previousX = mapX;
    int previousY = mapY;
    int previousZ = mapZ;

    RayHit result;
    for (int steps = 0; steps < 256; ++steps) {
        float distance = 0.0F;
        float side = 0.0F;
        if (sideX < sideY && sideX < sideZ) {
            distance = sideX;
            sideX += deltaX;
            previousX = mapX;
            previousY = mapY;
            previousZ = mapZ;
            mapX += stepX;
            side = 0.18F;
        } else if (sideY < sideZ) {
            distance = sideY;
            sideY += deltaY;
            previousX = mapX;
            previousY = mapY;
            previousZ = mapZ;
            mapY += stepY;
            side = 0.0F;
        } else {
            distance = sideZ;
            sideZ += deltaZ;
            previousX = mapX;
            previousY = mapY;
            previousZ = mapZ;
            mapZ += stepZ;
            side = 0.12F;
        }
        if (distance > maxDistance) {
            break;
        }
        const GcBlock block = blockAt(mapX, mapY, mapZ);
        const Position current{mapX, mapY, mapZ};
        const auto track = tracks_.find(current);
        if (track != tracks_.end()) {
            result.position = current;
            result.adjacent = Position{previousX, previousY, previousZ};
            result.distance = distance;
            result.side = side;
            result.hit = true;
            result.track = true;
            break;
        }
        if (block != GC_BLOCK_AIR) {
            const Position railAbove{mapX, mapY + 1, mapZ};
            if (tracks_.find(railAbove) != tracks_.end()) {
                result.position = railAbove;
                result.adjacent = Position{previousX, previousY, previousZ};
                result.distance = distance;
                result.side = side;
                result.hit = true;
                result.track = true;
                break;
            }
            result.block = block;
            result.position = Position{mapX, mapY, mapZ};
            result.adjacent = Position{previousX, previousY, previousZ};
            result.distance = distance;
            result.side = side;
            result.hit = true;
            break;
        }
    }
    return result;
}

bool Game::placeBlock(GcBlock block)
{
    if (block <= GC_BLOCK_AIR || block >= GC_BLOCK_COUNT) {
        return false;
    }
    const RayHit hit = castRay(yaw_, pitch_, 8.0F);
    if (!hit.hit) {
        return false;
    }
    const int placeX = hit.adjacent.x;
    const int placeY = hit.adjacent.y;
    const int placeZ = hit.adjacent.z;
    const Position target{placeX, placeY, placeZ};
    const bool overlapsPlayer =
        static_cast<float>(placeX + 1) > x_ - kPlayerRadius &&
        static_cast<float>(placeX) < x_ + kPlayerRadius &&
        static_cast<float>(placeY + 1) > y_ &&
        static_cast<float>(placeY) < y_ + kPlayerHeight &&
        static_cast<float>(placeZ + 1) > z_ - kPlayerRadius &&
        static_cast<float>(placeZ) < z_ + kPlayerRadius;
    if (blockAt(placeX, placeY, placeZ) != GC_BLOCK_AIR || overlapsPlayer ||
        tracks_.find(target) != tracks_.end()) {
        return false;
    }
    edits_[target] = block;
    syncShulkerBox(target, block);
    return true;
}

bool Game::applyBlockEdit(int x, int y, int z, GcBlock block)
{
    if (x < -1000000 || x > 1000000 || z < -1000000 || z > 1000000 ||
        y < -64 || y > 255 || block < GC_BLOCK_AIR ||
        block >= GC_BLOCK_COUNT) {
        return false;
    }
    const Position position{x, y, z};
    if (tracks_.find(position) != tracks_.end()) {
        return false;
    }
    edits_[position] = block;
    syncShulkerBox(position, block);
    return true;
}

bool Game::placeTrack(GcTrackType type, bool axisX)
{
    if (type < GC_TRACK_STRAIGHT || type >= GC_TRACK_COUNT) {
        return false;
    }
    const RayHit hit = castRay(yaw_, pitch_, 12.0F);
    if (!hit.hit || hit.track || hit.position.y >= 255) {
        return false;
    }
    const Position position{hit.position.x, hit.position.y + 1,
                            hit.position.z};
    return setTrackAt(position.x, position.y, position.z, type, axisX);
}

bool Game::setTrackAt(int x, int y, int z, GcTrackType type, bool axisX)
{
    if (x < -1000000 || x > 1000000 || z < -1000000 || z > 1000000 ||
        y < -63 || y > 255 || type < GC_TRACK_STRAIGHT ||
        type >= GC_TRACK_COUNT) {
        return false;
    }
    const Position position{x, y, z};
    const auto existing = tracks_.find(position);
    if (existing != tracks_.end()) {
        const bool occupied = std::any_of(
            vehicles_.begin(), vehicles_.end(),
            [position](const TrainVehicle &vehicle) {
                return vehicle.trackPosition == position;
            });
        if (occupied) {
            return false;
        }
    }
    Track track{position, type, axisX,
                existing == tracks_.end() ? false
                                          : existing->second.switchBranch};
    if (!trackSupportsVehicles(track)) {
        return false;
    }
    tracks_[position] = track;
    return true;
}

bool Game::removeTrackAt(int x, int y, int z)
{
    const Position position{x, y, z};
    const auto track = tracks_.find(position);
    if (track == tracks_.end()) {
        return false;
    }
    const bool occupied = std::any_of(
        vehicles_.begin(), vehicles_.end(),
        [position](const TrainVehicle &vehicle) {
            return vehicle.trackPosition == position;
        });
    if (occupied) {
        return false;
    }
    tracks_.erase(track);
    return true;
}

bool Game::getTrackAt(int x, int y, int z, Track &track) const
{
    const auto found = tracks_.find(Position{x, y, z});
    if (found == tracks_.end()) {
        return false;
    }
    track = found->second;
    return true;
}

bool Game::setTrackSwitch(int x, int y, int z, bool branch)
{
    const auto found = tracks_.find(Position{x, y, z});
    if (found == tracks_.end() || found->second.type != GC_TRACK_SWITCH) {
        return false;
    }
    found->second.switchBranch = branch;
    return true;
}

bool Game::placeVehicle(GcVehicleType type)
{
    if (type < GC_VEHICLE_BIG_BOY || type >= GC_VEHICLE_COUNT) {
        return false;
    }
    const RayHit hit = castRay(yaw_, pitch_, 16.0F);
    if (!hit.hit || !hit.track) {
        return false;
    }
    const auto trackIt = tracks_.find(hit.position);
    if (trackIt == tracks_.end() || !trackSupportsVehicles(trackIt->second)) {
        return false;
    }
    for (const auto &vehicle : vehicles_) {
        if (vehicle.trackPosition == hit.position) {
            return false;
        }
    }

    const bool facingX = trackIt->second.axisX;
    const int direction = 1;
    int consist = nextConsistId_++;
    int order = 0;
    const TrainVehicle *adjacent = nullptr;
    int adjacentDelta = 0;
    for (const auto &existing : vehicles_) {
        if (existing.facingX != facingX) {
            continue;
        }
        const int delta = facingX
            ? hit.position.x - existing.trackPosition.x
            : hit.position.z - existing.trackPosition.z;
        const int lateralDelta = facingX
            ? hit.position.z - existing.trackPosition.z
            : hit.position.x - existing.trackPosition.x;
        if (std::abs(delta) == 1 && lateralDelta == 0) {
            adjacent = &existing;
            adjacentDelta = delta;
            break;
        }
    }
    if (adjacent != nullptr) {
        consist = adjacent->consist;
        order = adjacent->consistOrder + adjacentDelta;
    }
    const bool locomotive = gameengine::isLocomotive(type);
    if (locomotive && adjacent != nullptr) {
        order = 0;
        for (auto &existing : vehicles_) {
            if (existing.consist != consist) {
                continue;
            }
            const int relative = facingX
                ? existing.trackPosition.x - hit.position.x
                : existing.trackPosition.z - hit.position.z;
            existing.consistOrder = relative * direction;
        }
    }

    GcTrainFuel fuelType = GC_TRAIN_FUEL_NONE;
    int fuel = 0;
    if (gameengine::isSteamLocomotive(type)) {
        fuelType = GC_TRAIN_FUEL_OIL;
    } else if (gameengine::isElectricLocomotive(type)) {
        fuelType = GC_TRAIN_FUEL_NONE;
    } else if (gameengine::isSteamTender(type)) {
        fuelType = GC_TRAIN_FUEL_OIL;
        fuel = 100;
    } else if (type == GC_VEHICLE_CSX_DIESEL ||
               type == GC_VEHICLE_NS_DIESEL ||
               type == GC_VEHICLE_BNSF_DIESEL ||
               type == GC_VEHICLE_UNION_PACIFIC_DIESEL) {
        fuelType = GC_TRAIN_FUEL_DIESEL;
        fuel = 100;
    }
    vehicles_.push_back(TrainVehicle{type, hit.position, facingX, direction,
                                     consist, order, 0, fuelType, fuel, false});
    return true;
}

bool Game::mineBlock()
{
    const RayHit hit = castRay(yaw_, pitch_, 8.0F);
    if (!hit.hit) {
        return false;
    }
    if (hit.track) {
        for (const auto &vehicle : vehicles_) {
            if (vehicle.trackPosition == hit.position) {
                return false;
            }
        }
        tracks_.erase(hit.position);
        return true;
    }
    edits_[hit.position] = GC_BLOCK_AIR;
    syncShulkerBox(hit.position, GC_BLOCK_AIR);
    return true;
}

void Game::drinkPotion()
{
    potionTicks_ = 3600;
}

void Game::drinkMilk()
{
    potionTicks_ = 0;
}

bool Game::eatFood(GcFood food)
{
    static constexpr int nourishment[GC_FOOD_COUNT] = {12, 3, 2, 10, 10, 12, 4};
    if (food < GC_FOOD_CAKE || food >= GC_FOOD_COUNT || foodLevel_ >= 100) {
        return false;
    }
    foodLevel_ = std::min(100, foodLevel_ + nourishment[food]);
    foodTickCounter_ = 0;
    return true;
}

void Game::setFlying(bool enabled)
{
    flying_ = enabled;
}

void Game::setPaused(bool paused)
{
    paused_ = paused;
}

void Game::tick()
{
    advanceTrains();
    if (!flying_) {
        verticalVelocity_ -= 0.008F;
        const float nextY = y_ + verticalVelocity_;
        if (collides(x_, nextY, z_)) {
            if (verticalVelocity_ < 0.0F) {
                y_ = std::ceil(y_) + 0.001F;
            }


            verticalVelocity_ = 0.0F;
        } else {
            y_ = nextY;
        }
    }
    if (potionTicks_ > 0) {
        --potionTicks_;
    }
    if (foodLevel_ > 0 && ++foodTickCounter_ >= 1800) {
        --foodLevel_;
        foodTickCounter_ = 0;
    }
}


void Game::advanceTrains()
{
    for (std::size_t engineIndex = 0; engineIndex < vehicles_.size(); ++engineIndex) {
        auto &engine = vehicles_[engineIndex];
        const bool locomotive = gameengine::isLocomotive(engine.type);
        if (!locomotive || !engine.powered) {
            continue;
        }

        auto tender = vehicles_.end();
        if (gameengine::isSteamLocomotive(engine.type)) {
            const GcVehicleType tenderType =
                gameengine::steamTenderFor(engine.type);
            tender = std::find_if(
                vehicles_.begin(), vehicles_.end(),
                [&engine, tenderType](const TrainVehicle &vehicle) {
                    return vehicle.consist == engine.consist &&
                           vehicle.type == tenderType;
                });
            engine.fuel = tender == vehicles_.end() ? 0 : tender->fuel;
        }
        if (engine.fuelType != GC_TRAIN_FUEL_NONE && engine.fuel <= 0) {
            engine.powered = false;
            continue;
        }

        const auto trackIt = tracks_.find(engine.trackPosition);
        if (trackIt == tracks_.end()) {
            engine.powered = false;
            continue;
        }
        const Position oldPosition = engine.trackPosition;
        int nextX = oldPosition.x + (engine.facingX ? engine.direction : 0);
        int nextZ = oldPosition.z + (engine.facingX ? 0 : engine.direction);
        Position nextPosition{nextX, oldPosition.y, nextZ};
        auto nextTrack = tracks_.find(nextPosition);

        if (trackIt->second.type == GC_TRACK_SWITCH &&
            trackIt->second.switchBranch) {
            const Position branch{
                oldPosition.x + (engine.facingX ? 0 : 1),
                oldPosition.y,
                oldPosition.z + (engine.facingX ? 1 : 0)};
            const auto branchTrack = tracks_.find(branch);
            if (branchTrack != tracks_.end()) {
                nextPosition = branch;
                nextTrack = branchTrack;
                engine.facingX = !engine.facingX;
                engine.direction = 1;
            }
        }
        if (nextTrack == tracks_.end() &&
            trackIt->second.type == GC_TRACK_HILL) {
            for (const int heightDelta : {1, -1}) {
                const Position slope{
                    oldPosition.x + (engine.facingX ? engine.direction : 0),
                    oldPosition.y + heightDelta,
                    oldPosition.z + (engine.facingX ? 0 : engine.direction)};
                const auto slopeTrack = tracks_.find(slope);
                if (slopeTrack != tracks_.end()) {
                    nextPosition = slope;
                    nextTrack = slopeTrack;
                    break;
                }
            }
        }
        if (nextTrack == tracks_.end() &&
            trackIt->second.type != GC_TRACK_STRAIGHT) {
            constexpr int offsets[4][2]{{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (const auto &offset : offsets) {
                const int candidateX = oldPosition.x + offset[0];
                const int candidateZ = oldPosition.z + offset[1];
                if (candidateX == oldPosition.x - (engine.facingX
                                                       ? engine.direction
                                                       : 0) &&
                    candidateZ == oldPosition.z - (engine.facingX
                                                       ? 0
                                                       : engine.direction)) {
                    continue;
                }
                const Position candidate{candidateX, oldPosition.y,
                                         candidateZ};
                const auto candidateTrack = tracks_.find(candidate);
                if (candidateTrack == tracks_.end()) {
                    continue;
                }
                if (trackIt->second.type == GC_TRACK_INTERSECTION ||
                    trackIt->second.type == GC_TRACK_TURNTABLE ||
                    trackIt->second.type == GC_TRACK_SWITCH ||
                    ((trackIt->second.type == GC_TRACK_CURVED ||
                      trackIt->second.type == GC_TRACK_CUSTOM_CURVE) &&
                     candidateTrack->second.axisX != engine.facingX)) {
                    nextPosition = candidate;
                    nextTrack = candidateTrack;
                    engine.facingX = offset[0] != 0;
                    engine.direction = (offset[0] != 0) ? offset[0] : offset[1];
                    break;
                }
            }
        }
        if (nextTrack == tracks_.end()) {
            engine.powered = false;
            continue;
        }

        const int tickRate = gameengine::isSteamLocomotive(engine.type) ? 18 : 12;
        if (++engine.fuelCounter < tickRate) {
            continue;
        }
        engine.fuelCounter = 0;
        const int moveX = nextPosition.x - oldPosition.x;
        const int moveY = nextPosition.y - oldPosition.y;
        const int moveZ = nextPosition.z - oldPosition.z;
        bool consistCanMove = true;
        for (const auto &vehicle : vehicles_) {
            if (vehicle.consist != engine.consist) {
                continue;
            }
            const Position target{vehicle.trackPosition.x + moveX,
                                 vehicle.trackPosition.y + moveY,
                                 vehicle.trackPosition.z + moveZ};
            if (tracks_.find(target) == tracks_.end()) {
                consistCanMove = false;
                break;
            }
            for (const auto &other : vehicles_) {
                if (other.consist != engine.consist &&
                    other.trackPosition == target) {
                    consistCanMove = false;
                    break;
                }
            }
            if (!consistCanMove) {
                break;
            }
        }
        if (!consistCanMove) {
            engine.powered = false;
            continue;
        }
        if (gameengine::isSteamLocomotive(engine.type)) {
            if (tender == vehicles_.end() || tender->fuel <= 0) {
                engine.powered = false;
                continue;
            }
            --tender->fuel;
            engine.fuel = tender->fuel;
        } else if (engine.fuelType != GC_TRAIN_FUEL_NONE) {
            --engine.fuel;
        }
        for (auto &vehicle : vehicles_) {
            if (vehicle.consist == engine.consist) {
                vehicle.trackPosition.x += moveX;
                vehicle.trackPosition.y += moveY;
                vehicle.trackPosition.z += moveZ;
                vehicle.facingX = engine.facingX;
                vehicle.direction = engine.direction;
            }
        }
    }
}

void Game::jump()
{
    if (flying_) {
        y_ += 0.5F;
        verticalVelocity_ = 0.0F;
    } else if (verticalVelocity_ == 0.0F && collides(x_, y_ - 0.02F, z_)) {
        verticalVelocity_ = 0.16F;
    }
}

void Game::look(float yawDelta, float pitchDelta)
{
    changeView(yawDelta, pitchDelta);
}

void Game::changeView(float yawDelta, float pitchDelta)
{
    yaw_ = std::fmod(yaw_ + yawDelta, 2.0F * kPi);
    if (yaw_ < 0.0F) {
        yaw_ += 2.0F * kPi;
    }
    pitch_ = std::clamp(pitch_ + pitchDelta, -1.2F, 1.2F);
}

float Game::playerX() const noexcept { return x_; }
float Game::playerY() const noexcept { return y_; }
float Game::playerZ() const noexcept { return z_; }
float Game::yaw() const noexcept { return yaw_; }
float Game::pitch() const noexcept { return pitch_; }
int Game::potionTicks() const noexcept { return potionTicks_; }
int Game::foodLevel() const noexcept { return foodLevel_; }
bool Game::flying() const noexcept { return flying_; }
bool Game::paused() const noexcept { return paused_; }

const std::unordered_map<Position, Track, PositionHash> &
Game::tracks() const noexcept
{
    return tracks_;
}

const std::vector<TrainVehicle> &Game::vehicles() const noexcept
{
    return vehicles_;
}

const std::unordered_map<Position, GcBlock, PositionHash> &
Game::blockEdits() const noexcept
{
    return edits_;
}

const std::vector<RemotePlayer> &Game::remotePlayers() const noexcept
{
    return remotePlayers_;
}

void Game::setRemotePlayer(int id, float x, float y, float z, float yaw,
                           float pitch)
{
    if (id <= 0 || !std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(z) || !std::isfinite(yaw) || !std::isfinite(pitch) ||
        std::abs(x) >= 1000000.0F || std::abs(y) >= 1000000.0F ||
        std::abs(z) >= 1000000.0F) {
        return;
    }
    const auto found = std::find_if(
        remotePlayers_.begin(), remotePlayers_.end(),
        [id](const RemotePlayer &player) { return player.id == id; });
    const RemotePlayer player{id, x, y, z, yaw, pitch};
    if (found == remotePlayers_.end()) {
        if (remotePlayers_.size() < 8U) {
            remotePlayers_.push_back(player);
        }
    } else {
        *found = player;
    }
}

void Game::removeRemotePlayer(int id)
{
    remotePlayers_.erase(
        std::remove_if(remotePlayers_.begin(), remotePlayers_.end(),
                       [id](const RemotePlayer &player) {
                           return player.id == id;
                       }),
        remotePlayers_.end());
}

bool Game::addTrainFuel(int amount)
{
    const RayHit hit = castRay(yaw_, pitch_, 16.0F);
    if (!hit.hit || !hit.track) {
        return false;
    }
    for (auto &vehicle : vehicles_) {
        if (!(vehicle.trackPosition == hit.position)) {
            continue;
        }
        return loadTrainFuel(vehicle.fuelType, amount);
    }
    return false;
}

bool Game::loadTrainFuel(GcTrainFuel type, int amount)
{
    constexpr int kFuelCapacity = 1000;
    if ((type != GC_TRAIN_FUEL_OIL && type != GC_TRAIN_FUEL_DIESEL) ||
        amount <= 0) {
        return false;
    }
    const RayHit hit = castRay(yaw_, pitch_, 16.0F);
    if (!hit.hit || !hit.track) {
        return false;
    }
    for (auto &vehicle : vehicles_) {
        if (!(vehicle.trackPosition == hit.position) ||
            vehicle.fuelType != type || vehicle.fuel >= kFuelCapacity) {
            continue;
        }
        const bool compatible =
            (type == GC_TRAIN_FUEL_OIL &&
             gameengine::isSteamTender(vehicle.type)) ||
            (type == GC_TRAIN_FUEL_DIESEL &&
             (vehicle.type == GC_VEHICLE_CSX_DIESEL ||
              vehicle.type == GC_VEHICLE_NS_DIESEL ||
              vehicle.type == GC_VEHICLE_BNSF_DIESEL ||
              vehicle.type == GC_VEHICLE_UNION_PACIFIC_DIESEL));
        if (!compatible) {
            return false;
        }
        vehicle.fuel += std::min(amount, kFuelCapacity - vehicle.fuel);
        return true;
    }
    return false;
}

void Game::setTrainPower(bool enabled)
{
    std::vector<int> poweredConsists;
    for (auto &vehicle : vehicles_) {
        const bool locomotive = gameengine::isLocomotive(vehicle.type);
        if (!locomotive) {
            continue;
        }
        vehicle.powered = enabled &&
            std::find(poweredConsists.begin(), poweredConsists.end(),
                      vehicle.consist) == poweredConsists.end();
        if (vehicle.powered) {
            poweredConsists.push_back(vehicle.consist);
        }
    }
}

void Game::toggleTrainPower()
{
    const bool currentlyPowered = std::any_of(
        vehicles_.begin(), vehicles_.end(), [](const TrainVehicle &vehicle) {
            return vehicle.powered && gameengine::isLocomotive(vehicle.type);
        });
    setTrainPower(!currentlyPowered);
}

void Game::toggleTrackSwitch()
{
    const RayHit hit = castRay(yaw_, pitch_, 16.0F);
    if (!hit.hit || !hit.track) {
        return;
    }
    const auto track = tracks_.find(hit.position);
    if (track != tracks_.end() && track->second.type == GC_TRACK_SWITCH) {
        track->second.switchBranch = !track->second.switchBranch;
    }
}

}
