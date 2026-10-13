#include "engine.hpp"

#include <cstdint>
#include <new>

struct GcGame {
    explicit GcGame(unsigned int seed) : game(seed) {}
    glitchcraft::Game game;
};

extern "C" {

GcGame *gc_game_create(unsigned int seed)
{
    return new (std::nothrow) GcGame(seed);
}

void gc_game_destroy(GcGame *game)
{
    delete game;
}

GcBlock gc_game_get_block(const GcGame *game, int x, int y, int z)
{
    return game == nullptr ? GC_BLOCK_AIR : game->game.blockAt(x, y, z);
}

void gc_game_get_player(const GcGame *game, GcPlayerState *state)
{
    if (game == nullptr || state == nullptr) {
        return;
    }
    state->x = game->game.playerX();
    state->z = game->game.playerZ();
    state->yaw = game->game.yaw();
    state->pitch = game->game.pitch();
    state->potion_ticks = game->game.potionTicks();
    state->flying = game->game.flying() ? 1 : 0;
    state->paused = game->game.paused() ? 1 : 0;
    state->food_level = game->game.foodLevel();
}

int gc_game_move(GcGame *game, float forward, float strafe)
{
    return game != nullptr && game->game.move(forward, strafe) ? 1 : 0;
}

void gc_game_tick(GcGame *game)
{
    if (game != nullptr) {
        game->game.tick();
    }
}

int gc_game_place_block(GcGame *game, GcBlock block)
{
    return game != nullptr && game->game.placeBlock(block) ? 1 : 0;
}

int gc_game_mine_block(GcGame *game)
{
    return game != nullptr && game->game.mineBlock() ? 1 : 0;
}

void gc_game_drink_potion(GcGame *game)
{
    if (game != nullptr) {
        game->game.drinkPotion();
    }
}

void gc_game_drink_milk(GcGame *game)
{
    if (game != nullptr) {
        game->game.drinkMilk();
    }
}

int gc_game_eat_food(GcGame *game, GcFood food)
{
    return game != nullptr && game->game.eatFood(food) ? 1 : 0;
}

void gc_game_set_flying(GcGame *game, int enabled)
{
    if (game != nullptr) {
        game->game.setFlying(enabled != 0);
    }
}

void gc_game_set_paused(GcGame *game, int paused)
{
    if (game != nullptr) {
        game->game.setPaused(paused != 0);
    }
}

int gc_game_place_track(GcGame *game, GcTrackType type, int axis_x)
{
    return game != nullptr &&
                   game->game.placeTrack(type, axis_x != 0)
               ? 1
               : 0;
}

int gc_game_place_vehicle(GcGame *game, GcVehicleType type)
{
    return game != nullptr && game->game.placeVehicle(type) ? 1 : 0;
}

int gc_game_add_train_fuel(GcGame *game, int amount)
{
    return game != nullptr && game->game.addTrainFuel(amount) ? 1 : 0;
}

void gc_game_set_train_power(GcGame *game, int enabled)
{
    if (game != nullptr) {
        game->game.setTrainPower(enabled != 0);
    }
}

void gc_game_toggle_train_power(GcGame *game)
{
    if (game != nullptr) {
        game->game.toggleTrainPower();
    }
}

void gc_game_toggle_track_switch(GcGame *game)
{
    if (game != nullptr) {
        game->game.toggleTrackSwitch();
    }
}

int gc_game_track_count(const GcGame *game)
{
    return game == nullptr ? 0 :
        static_cast<int>(game->game.tracks().size());
}

int gc_game_vehicle_count(const GcGame *game)
{
    return game == nullptr ? 0 :
        static_cast<int>(game->game.vehicles().size());
}

int gc_game_get_track(const GcGame *game, int index, GcTrackState *state)
{
    if (game == nullptr || state == nullptr || index < 0 ||
        static_cast<std::size_t>(index) >= game->game.tracks().size()) {
        return 0;
    }
    auto track = game->game.tracks().begin();
    std::advance(track, index);
    state->x = track->second.position.x;
    state->y = track->second.position.y;
    state->z = track->second.position.z;
    state->type = track->second.type;
    state->axis_x = track->second.axisX ? 1 : 0;
    state->switch_branch = track->second.switchBranch ? 1 : 0;
    return 1;
}

int gc_game_get_vehicle(const GcGame *game, int index,
                        GcVehicleState *state)
{
    if (game == nullptr || state == nullptr || index < 0 ||
        static_cast<std::size_t>(index) >= game->game.vehicles().size()) {
        return 0;
    }
    const auto &vehicle =
        game->game.vehicles()[static_cast<std::size_t>(index)];
    state->type = vehicle.type;
    state->track_x = vehicle.trackPosition.x;
    state->track_y = vehicle.trackPosition.y;
    state->track_z = vehicle.trackPosition.z;
    state->facing_x = vehicle.facingX ? 1 : 0;
    state->direction = vehicle.direction;
    state->consist_id = vehicle.consist;
    state->fuel_type = vehicle.fuelType;
    state->fuel = vehicle.fuel;
    state->powered = vehicle.powered ? 1 : 0;
    return 1;
}

}
