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

    const float3 origin = params.camera_origin;
    const float3 raw_direction = make_float3(screen.x * params.view_scale,
                                             screen.y * params.view_scale,
                                             -2.5f * params.view_scale);
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
               OPTIX_RAY_TYPE_RADIANCE, OPTIX_RAY_TYPE_COUNT,
               OPTIX_RAY_TYPE_RADIANCE, red, green, blue);

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

extern "C" __global__ void __miss__shadow() {
    optixSetPayload_0(1u);
}

extern "C" __global__ void __closesthit__shadow() {
    optixSetPayload_0(0u);
}

extern "C" __global__ void __closesthit__lit() {
    const unsigned int primitive = optixGetPrimitiveIndex();
    const uint3 triangle = params.indices[primitive];
    const float3 a = params.vertices[triangle.x];
    const float3 b = params.vertices[triangle.y];
    const float3 c = params.vertices[triangle.z];
    const float3 edge_ab = make_float3(b.x - a.x, b.y - a.y, b.z - a.z);
    const float3 edge_ac = make_float3(c.x - a.x, c.y - a.y, c.z - a.z);
    float3 normal = make_float3(edge_ab.y * edge_ac.z - edge_ab.z * edge_ac.y,
                                edge_ab.z * edge_ac.x - edge_ab.x * edge_ac.z,
                                edge_ab.x * edge_ac.y - edge_ab.y * edge_ac.x);
    const float inverse_normal_length = rsqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    normal = make_float3(normal.x * inverse_normal_length, normal.y * inverse_normal_length,
                         normal.z * inverse_normal_length);
    const float3 incoming = optixGetWorldRayDirection();
    if (normal.x * incoming.x + normal.y * incoming.y + normal.z * incoming.z > 0.0f)
        normal = make_float3(-normal.x, -normal.y, -normal.z);

    const float distance = optixGetRayTmax();
    const float3 ray_origin = optixGetWorldRayOrigin();
    const float3 hit = make_float3(ray_origin.x + incoming.x * distance,
                                   ray_origin.y + incoming.y * distance,
                                   ray_origin.z + incoming.z * distance);
    const float3 to_light = make_float3(params.light_position.x - hit.x,
                                        params.light_position.y - hit.y,
                                        params.light_position.z - hit.z);
    const float light_distance = sqrtf(to_light.x * to_light.x + to_light.y * to_light.y + to_light.z * to_light.z);
    const float3 light_direction = make_float3(to_light.x / light_distance, to_light.y / light_distance,
                                               to_light.z / light_distance);
    unsigned int visible = 0;
    optixTrace(params.handle,
               make_float3(hit.x + normal.x * 1.0e-3f, hit.y + normal.y * 1.0e-3f, hit.z + normal.z * 1.0e-3f),
               light_direction, 0.0f, light_distance - 1.0e-3f, 0.0f,
               OptixVisibilityMask(255), OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT | OPTIX_RAY_FLAG_DISABLE_ANYHIT,
               OPTIX_RAY_TYPE_SHADOW, OPTIX_RAY_TYPE_COUNT, OPTIX_RAY_TYPE_SHADOW, visible);

    const float diffuse = fmaxf(0.0f, normal.x * light_direction.x + normal.y * light_direction.y +
                                      normal.z * light_direction.z);
    const float lighting = 0.10f + (visible ? 0.90f * diffuse : 0.0f);
    const float3 base_color = params.primitive_colors[primitive];
    set_color(base_color.x * lighting, base_color.y * lighting, base_color.z * lighting);
}
