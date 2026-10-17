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
    GC_BLOCK_SHULKER_WHITE,
    GC_BLOCK_SHULKER_ORANGE,
    GC_BLOCK_SHULKER_MAGENTA,
    GC_BLOCK_SHULKER_LIGHT_BLUE,
    GC_BLOCK_SHULKER_YELLOW,
    GC_BLOCK_SHULKER_LIME,
    GC_BLOCK_SHULKER_PINK,
    GC_BLOCK_SHULKER_GRAY,
    GC_BLOCK_SHULKER_LIGHT_GRAY,
    GC_BLOCK_SHULKER_CYAN,
    GC_BLOCK_SHULKER_PURPLE,
    GC_BLOCK_SHULKER_BLUE,
    GC_BLOCK_SHULKER_BROWN,
    GC_BLOCK_SHULKER_GREEN,
    GC_BLOCK_SHULKER_RED,
    GC_BLOCK_SHULKER_BLACK,
    GC_BLOCK_ASPHALT,
    GC_BLOCK_ROAD_STRAIGHT,
    GC_BLOCK_ROAD_CURVE,
    GC_BLOCK_ROAD_T_INTERSECTION,
    GC_BLOCK_ROAD_CROSS_INTERSECTION,
    GC_BLOCK_ROAD_DEAD_END,
    GC_BLOCK_ROAD_ROUNDABOUT,
    GC_BLOCK_COUNT
} GcBlock;

typedef struct GcShulkerStack {
    GcBlock block;
    int count;
} GcShulkerStack;

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
    GC_VEHICLE_SHINKANSEN_ENGINE_RED,
    GC_VEHICLE_SHINKANSEN_ENGINE_YELLOW,
    GC_VEHICLE_SHINKANSEN_ENGINE_BLUE,
    GC_VEHICLE_SHINKANSEN_ENGINE_GREEN,
    GC_VEHICLE_SHINKANSEN_ENGINE_LIGHT_BLUE,
    GC_VEHICLE_SHINKANSEN_ENGINE_CYAN,
    GC_VEHICLE_SHINKANSEN_ENGINE_WHITE,
    GC_VEHICLE_SHINKANSEN_ENGINE_GRAY,
    GC_VEHICLE_SHINKANSEN_ENGINE_BLACK,
    GC_VEHICLE_SHINKANSEN_REAR_ENGINE_RED,
    GC_VEHICLE_SHINKANSEN_REAR_ENGINE_YELLOW,
    GC_VEHICLE_SHINKANSEN_REAR_ENGINE_BLUE,
    GC_VEHICLE_SHINKANSEN_REAR_ENGINE_GREEN,
    GC_VEHICLE_SHINKANSEN_REAR_ENGINE_LIGHT_BLUE,
    GC_VEHICLE_SHINKANSEN_REAR_ENGINE_CYAN,
    GC_VEHICLE_SHINKANSEN_REAR_ENGINE_WHITE,
    GC_VEHICLE_SHINKANSEN_REAR_ENGINE_GRAY,
    GC_VEHICLE_SHINKANSEN_REAR_ENGINE_BLACK,
    GC_VEHICLE_SHINKANSEN_COACH_RED,
    GC_VEHICLE_SHINKANSEN_COACH_YELLOW,
    GC_VEHICLE_SHINKANSEN_COACH_BLUE,
    GC_VEHICLE_SHINKANSEN_COACH_GREEN,
    GC_VEHICLE_SHINKANSEN_COACH_LIGHT_BLUE,
    GC_VEHICLE_SHINKANSEN_COACH_CYAN,
    GC_VEHICLE_SHINKANSEN_COACH_WHITE,
    GC_VEHICLE_SHINKANSEN_COACH_GRAY,
    GC_VEHICLE_SHINKANSEN_COACH_BLACK,
    GC_VEHICLE_SUBWAY_ENGINE_A,
    GC_VEHICLE_SUBWAY_ENGINE_B,
    GC_VEHICLE_SUBWAY_ENGINE_C,
    GC_VEHICLE_SUBWAY_ENGINE_D,
    GC_VEHICLE_SUBWAY_ENGINE_E,
    GC_VEHICLE_SUBWAY_ENGINE_F,
    GC_VEHICLE_SUBWAY_ENGINE_G,
    GC_VEHICLE_SUBWAY_ENGINE_H,
    GC_VEHICLE_SUBWAY_ENGINE_I,
    GC_VEHICLE_SUBWAY_ENGINE_J,
    GC_VEHICLE_SUBWAY_ENGINE_K,
    GC_VEHICLE_SUBWAY_ENGINE_L,
    GC_VEHICLE_SUBWAY_ENGINE_M,
    GC_VEHICLE_SUBWAY_ENGINE_N,
    GC_VEHICLE_SUBWAY_ENGINE_O,
    GC_VEHICLE_SUBWAY_ENGINE_P,
    GC_VEHICLE_SUBWAY_ENGINE_Q,
    GC_VEHICLE_SUBWAY_ENGINE_R,
    GC_VEHICLE_SUBWAY_ENGINE_S,
    GC_VEHICLE_SUBWAY_ENGINE_T,
    GC_VEHICLE_SUBWAY_ENGINE_U,
    GC_VEHICLE_SUBWAY_ENGINE_V,
    GC_VEHICLE_SUBWAY_ENGINE_W,
    GC_VEHICLE_SUBWAY_ENGINE_X,
    GC_VEHICLE_SUBWAY_ENGINE_Y,
    GC_VEHICLE_SUBWAY_ENGINE_Z,
    GC_VEHICLE_SUBWAY_COACH_A,
    GC_VEHICLE_SUBWAY_COACH_B,
    GC_VEHICLE_SUBWAY_COACH_C,
    GC_VEHICLE_SUBWAY_COACH_D,
    GC_VEHICLE_SUBWAY_COACH_E,
    GC_VEHICLE_SUBWAY_COACH_F,
    GC_VEHICLE_SUBWAY_COACH_G,
    GC_VEHICLE_SUBWAY_COACH_H,
    GC_VEHICLE_SUBWAY_COACH_I,
    GC_VEHICLE_SUBWAY_COACH_J,
    GC_VEHICLE_SUBWAY_COACH_K,
    GC_VEHICLE_SUBWAY_COACH_L,
    GC_VEHICLE_SUBWAY_COACH_M,
    GC_VEHICLE_SUBWAY_COACH_N,
    GC_VEHICLE_SUBWAY_COACH_O,
    GC_VEHICLE_SUBWAY_COACH_P,
    GC_VEHICLE_SUBWAY_COACH_Q,
    GC_VEHICLE_SUBWAY_COACH_R,
    GC_VEHICLE_SUBWAY_COACH_S,
    GC_VEHICLE_SUBWAY_COACH_T,
    GC_VEHICLE_SUBWAY_COACH_U,
    GC_VEHICLE_SUBWAY_COACH_V,
    GC_VEHICLE_SUBWAY_COACH_W,
    GC_VEHICLE_SUBWAY_COACH_X,
    GC_VEHICLE_SUBWAY_COACH_Y,
    GC_VEHICLE_SUBWAY_COACH_Z,
    GC_VEHICLE_FLYING_SCOTSMAN,
    GC_VEHICLE_FLYING_SCOTSMAN_TENDER,
    GC_VEHICLE_FLYING_SCOTSMAN_COACH,
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
int gc_game_apply_block_edit(GcGame *game, int x, int y, int z,
                             GcBlock block);
int gc_game_mine_block(GcGame *game);
void gc_game_drink_potion(GcGame *game);
void gc_game_drink_milk(GcGame *game);
int gc_game_eat_food(GcGame *game, GcFood food);
int gc_game_get_shulker_slot(const GcGame *game, int x, int y, int z,
                             int slot, GcShulkerStack *stack);
int gc_game_set_shulker_slot(GcGame *game, int x, int y, int z, int slot,
                             GcBlock block, int count);
void gc_game_set_flying(GcGame *game, int enabled);
void gc_game_set_paused(GcGame *game, int paused);
int gc_game_place_track(GcGame *game, GcTrackType type, int axis_x);
int gc_game_set_track_at(GcGame *game, int x, int y, int z,
                         GcTrackType type, int axis_x);
int gc_game_remove_track_at(GcGame *game, int x, int y, int z);
int gc_game_get_track_at(const GcGame *game, int x, int y, int z,
                         GcTrackState *state);
int gc_game_set_track_switch(GcGame *game, int x, int y, int z,
                             int branch);
int gc_game_place_vehicle(GcGame *game, GcVehicleType type);
int gc_game_add_train_fuel(GcGame *game, int amount);
int gc_game_load_train_fuel(GcGame *game, GcTrainFuel type, int amount);
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
