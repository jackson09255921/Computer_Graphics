#include <optix_device.h>

#include "optix_triangle_shared.h"

extern "C" {
__constant__ OptixTriangleParams params;
}

static __forceinline__ __device__ void set_color(float r, float g, float b) {
    optixSetPayload_0(__float_as_uint(r));
    optixSetPayload_1(__float_as_uint(g));
    optixSetPayload_2(__float_as_uint(b));
}

extern "C" __global__ void __raygen__triangle() {
    const uint3 index = optixGetLaunchIndex();
    const uint3 dimensions = optixGetLaunchDimensions();
    const float2 screen = make_float2(
        (2.0f * (static_cast<float>(index.x) + 0.5f) / static_cast<float>(dimensions.x) - 1.0f) *
            (static_cast<float>(dimensions.x) / static_cast<float>(dimensions.y)),
        2.0f * (static_cast<float>(index.y) + 0.5f) / static_cast<float>(dimensions.y) - 1.0f);

    const float3 origin = make_float3(0.0f, 0.0f, 2.0f);
    const float3 raw_direction = make_float3(screen.x, screen.y, -2.0f);
    const float inverse_length = rsqrtf(raw_direction.x * raw_direction.x +
                                        raw_direction.y * raw_direction.y +
                                        raw_direction.z * raw_direction.z);
    const float3 direction = make_float3(raw_direction.x * inverse_length,
                                         raw_direction.y * inverse_length,
                                         raw_direction.z * inverse_length);

    unsigned int red = 0;
    unsigned int green = 0;
    unsigned int blue = 0;
    optixTrace(params.handle, origin, direction, 0.0f, 1.0e16f, 0.0f,
               OptixVisibilityMask(255), OPTIX_RAY_FLAG_NONE,
               0, 1, 0, red, green, blue);

    const float r = __uint_as_float(red);
    const float g = __uint_as_float(green);
    const float b = __uint_as_float(blue);
    params.image[index.y * params.width + index.x] = make_uchar4(
        static_cast<unsigned char>(fminf(fmaxf(r, 0.0f), 1.0f) * 255.0f),
        static_cast<unsigned char>(fminf(fmaxf(g, 0.0f), 1.0f) * 255.0f),
        static_cast<unsigned char>(fminf(fmaxf(b, 0.0f), 1.0f) * 255.0f), 255);
}

extern "C" __global__ void __miss__background() {
    set_color(0.03f, 0.05f, 0.10f);
}

extern "C" __global__ void __closesthit__barycentric() {
    const float2 barycentric = optixGetTriangleBarycentrics();
    set_color(1.0f - barycentric.x - barycentric.y, barycentric.x, barycentric.y);
}
