#pragma once

#include <cuda_runtime.h>
#include <optix.h>

struct OptixTriangleParams {
    uchar4* image;
    unsigned int width;
    unsigned int height;
    OptixTraversableHandle handle;
    const float3* primitive_colors;
    float3 camera_origin;
    float view_scale;
};
