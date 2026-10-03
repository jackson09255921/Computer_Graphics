#pragma once

#include <cuda_runtime.h>
#include <optix.h>

struct OptixTriangleParams {
    uchar4* image;
    unsigned int width;
    unsigned int height;
    OptixTraversableHandle handle;
    const float3* vertices;
    const uint3* indices;
    const float3* primitive_colors;
    float3 camera_origin;
    float view_scale;
    float3 light_position;
};

enum OptixRayType : unsigned int {
    OPTIX_RAY_TYPE_RADIANCE = 0,
    OPTIX_RAY_TYPE_SHADOW = 1,
    OPTIX_RAY_TYPE_COUNT = 2
};
