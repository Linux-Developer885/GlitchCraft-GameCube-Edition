#ifndef GLITCHCRAFT_GAME_API_H
#define GLITCHCRAFT_GAME_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GcGame GcGame;

typedef enum GcBlock {
    GC_BLOCK_AIR = 0,
    GC_BLOCK_GRASS,
    GC_BLOCK_DIRT,
    GC_BLOCK_STONE,
    GC_BLOCK_WOOD,
    GC_BLOCK_WATER,
    GC_BLOCK_LAVA,
    GC_BLOCK_REDSTONE,
    GC_BLOCK_COUNT
} GcBlock;

typedef enum GcFood {
    GC_FOOD_CAKE = 0,
    GC_FOOD_COOKIE,
    GC_FOOD_CRACKERS,
    GC_FOOD_SANDWICH,
    GC_FOOD_SPAGHETTI,
    GC_FOOD_PIZZA,
    GC_FOOD_COTTON_CANDY,
    GC_FOOD_COUNT
} GcFood;

typedef enum GcTrackType {
    GC_TRACK_STRAIGHT = 0,
    GC_TRACK_CURVED,
    GC_TRACK_INTERSECTION,
    GC_TRACK_CUSTOM_CURVE,
    GC_TRACK_SWITCH,
    GC_TRACK_HILL,
    GC_TRACK_TURNTABLE,
    GC_TRACK_COUNT
} GcTrackType;

typedef enum GcVehicleType {
    GC_VEHICLE_BIG_BOY = 0,
    GC_VEHICLE_CSX_DIESEL,
    GC_VEHICLE_NS_DIESEL,
    GC_VEHICLE_BNSF_DIESEL,
    GC_VEHICLE_UNION_PACIFIC_DIESEL,
    GC_VEHICLE_BIG_BOY_TENDER,
    GC_VEHICLE_BOXCAR,
    GC_VEHICLE_HOPPER_CAR,
    GC_VEHICLE_GONDOLA_CAR,
    GC_VEHICLE_FLATCAR,
    GC_VEHICLE_TANK_CAR,
    GC_VEHICLE_CABOOSE,
    GC_VEHICLE_AUTORACK_CAR,
    GC_VEHICLE_COUNT
} GcVehicleType;

typedef enum GcTrainFuel {
    GC_TRAIN_FUEL_NONE = 0,
    GC_TRAIN_FUEL_OIL,
    GC_TRAIN_FUEL_DIESEL
} GcTrainFuel;

typedef struct GcPlayerState {
    float x;
    float z;
    float yaw;
    float pitch;
    int potion_ticks;
    int flying;
    int paused;
    int food_level;
} GcPlayerState;

typedef struct GcTrackState {
    int x;
    int y;
    int z;
    GcTrackType type;
    int axis_x;
    int switch_branch;
} GcTrackState;

typedef struct GcVehicleState {
    GcVehicleType type;
    int track_x;
    int track_y;
    int track_z;
    int facing_x;
    int direction;
    int consist_id;
    GcTrainFuel fuel_type;
    int fuel;
    int powered;
} GcVehicleState;

GcGame *gc_game_create(unsigned int seed);
void gc_game_destroy(GcGame *game);
GcBlock gc_game_get_block(const GcGame *game, int x, int y, int z);
void gc_game_get_player(const GcGame *game, GcPlayerState *state);
int gc_game_move(GcGame *game, float forward, float strafe);
void gc_game_tick(GcGame *game);
int gc_game_place_block(GcGame *game, GcBlock block);
int gc_game_mine_block(GcGame *game);
void gc_game_drink_potion(GcGame *game);
void gc_game_drink_milk(GcGame *game);
int gc_game_eat_food(GcGame *game, GcFood food);
void gc_game_set_flying(GcGame *game, int enabled);
void gc_game_set_paused(GcGame *game, int paused);
int gc_game_place_track(GcGame *game, GcTrackType type, int axis_x);
int gc_game_place_vehicle(GcGame *game, GcVehicleType type);
int gc_game_add_train_fuel(GcGame *game, int amount);
void gc_game_set_train_power(GcGame *game, int enabled);
void gc_game_toggle_train_power(GcGame *game);
void gc_game_toggle_track_switch(GcGame *game);
int gc_game_track_count(const GcGame *game);
int gc_game_vehicle_count(const GcGame *game);
int gc_game_get_track(const GcGame *game, int index, GcTrackState *state);
int gc_game_get_vehicle(const GcGame *game, int index, GcVehicleState *state);

float gc_voxel_dot4(const float lhs[4], const float rhs[4]);

#ifdef __cplusplus
}
#endif

#endif
