#ifndef GLITCHCRAFT_VOXEL_KERNELS_H
#define GLITCHCRAFT_VOXEL_KERNELS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

float gc_engine_dot4(const float lhs[4], const float rhs[4]);
void gc_engine_clear_depth(float *depth, size_t count, float value);

#ifdef __cplusplus
}
#endif

#endif
