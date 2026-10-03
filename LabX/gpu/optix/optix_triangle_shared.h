#pragma once

#include <cuda_runtime.h>
#include <optix.h>

struct OptixTriangleParams {
    uchar4* image;
    float4* accumulation;
    float3* gbuffer_normal;
    float* gbuffer_depth;
    float3* gbuffer_albedo;
    float2* gbuffer_motion;
    const float3* previous_normal;
    const float* previous_depth;
    const float3* previous_albedo;
    unsigned int* temporal_validity;
    unsigned int width;
    unsigned int height;
    unsigned int sample_index;
    unsigned int frame_index;
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
