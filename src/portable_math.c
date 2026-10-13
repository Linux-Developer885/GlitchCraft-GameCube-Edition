#include "glitchcraft/game_api.h"
#include "GameEngine/voxel_kernels.h"

float gc_voxel_dot4(const float lhs[4], const float rhs[4])
{
    return gc_engine_dot4(lhs, rhs);
}
