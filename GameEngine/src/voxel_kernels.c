#include "GameEngine/voxel_kernels.h"

float gc_engine_dot4(const float lhs[4], const float rhs[4])
{
    if (lhs == 0 || rhs == 0) {
        return 0.0F;
    }
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] +
           lhs[2] * rhs[2] + lhs[3] * rhs[3];
}

void gc_engine_clear_depth(float *depth, size_t count, float value)
{
    if (depth == 0) {
        return;
    }
    for (size_t index = 0; index < count; ++index) {
        depth[index] = value;
    }
}
