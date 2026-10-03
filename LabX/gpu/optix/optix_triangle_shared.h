#pragma once

#include <cuda_runtime.h>
#include <optix.h>

struct OptixTriangleParams {
    uchar4* image;
    float4* accumulation;
    unsigned int width;
    unsigned int height;
    unsigned int sample_index;
    OptixTraversableHandle handle;
    const float3* vertices;
    const uint3* indices;
    const float3* primitive_colors;
    const float* primitive_reflectivity;
    const float* primitive_roughness;
    float3 camera_origin;
    float view_scale;
    float3 light_position;
};

enum OptixRayType : unsigned int {
    OPTIX_RAY_TYPE_RADIANCE = 0,
    OPTIX_RAY_TYPE_SHADOW = 1,
    OPTIX_RAY_TYPE_COUNT = 2
};
