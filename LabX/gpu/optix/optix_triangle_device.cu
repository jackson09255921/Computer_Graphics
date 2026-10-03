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

static __forceinline__ __device__ unsigned int hash(unsigned int value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}

static __forceinline__ __device__ float random_float(unsigned int& state) {
    state = hash(state);
    return (static_cast<float>(state & 0x00ffffffu) + 0.5f) * (1.0f / 16777216.0f);
}

static __forceinline__ __device__ float3 normalize_vector(float3 value) {
    const float inverse_length = rsqrtf(value.x * value.x + value.y * value.y + value.z * value.z);
    return make_float3(value.x * inverse_length, value.y * inverse_length, value.z * inverse_length);
}

static __forceinline__ __device__ float3 cross_vector(float3 lhs, float3 rhs) {
    return make_float3(lhs.y * rhs.z - lhs.z * rhs.y,
                       lhs.z * rhs.x - lhs.x * rhs.z,
                       lhs.x * rhs.y - lhs.y * rhs.x);
}

static __forceinline__ __device__ float3 sample_ggx_half_vector(float3 normal, float roughness,
                                                                unsigned int& random_state) {
    const float alpha = fmaxf(roughness * roughness, 0.001f);
    const float alpha_squared = alpha * alpha;
    const float first = random_float(random_state);
    const float second = random_float(random_state);
    const float phi = 6.28318530718f * first;
    const float cosine = sqrtf((1.0f - second) / (1.0f + (alpha_squared - 1.0f) * second));
    const float sine = sqrtf(fmaxf(0.0f, 1.0f - cosine * cosine));
    const float3 helper = fabsf(normal.z) < 0.999f ? make_float3(0.0f, 0.0f, 1.0f)
                                                    : make_float3(1.0f, 0.0f, 0.0f);
    const float3 tangent = normalize_vector(cross_vector(helper, normal));
    const float3 bitangent = cross_vector(normal, tangent);
    return normalize_vector(make_float3(
        tangent.x * cosf(phi) * sine + bitangent.x * sinf(phi) * sine + normal.x * cosine,
        tangent.y * cosf(phi) * sine + bitangent.y * sinf(phi) * sine + normal.y * cosine,
        tangent.z * cosf(phi) * sine + bitangent.z * sinf(phi) * sine + normal.z * cosine));
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

    const unsigned int pixel = index.y * params.width + index.x;
    params.gbuffer_normal[pixel] = make_float3(0.0f, 0.0f, 0.0f);
    params.gbuffer_depth[pixel] = 0.0f;
    params.gbuffer_albedo[pixel] = make_float3(0.0f, 0.0f, 0.0f);
    params.gbuffer_motion[pixel] = make_float2(0.0f, 0.0f);

    unsigned int red = 0;
    unsigned int green = 0;
    unsigned int blue = 0;
    unsigned int depth = 0;
    optixTrace(params.handle, origin, direction, 0.0f, 1.0e16f, 0.0f,
               OptixVisibilityMask(255), OPTIX_RAY_FLAG_NONE,
               OPTIX_RAY_TYPE_RADIANCE, OPTIX_RAY_TYPE_COUNT,
               OPTIX_RAY_TYPE_RADIANCE, red, green, blue, depth);

    const float r = __uint_as_float(red);
    const float g = __uint_as_float(green);
    const float b = __uint_as_float(blue);
    float4 accumulated = params.accumulation[pixel];
    accumulated.x += r;
    accumulated.y += g;
    accumulated.z += b;
    accumulated.w += 1.0f;
    params.accumulation[pixel] = accumulated;
    const float inverse_samples = 1.0f / accumulated.w;
    params.image[pixel] = make_uchar4(
        static_cast<unsigned char>(fminf(fmaxf(accumulated.x * inverse_samples, 0.0f), 1.0f) * 255.0f),
        static_cast<unsigned char>(fminf(fmaxf(accumulated.y * inverse_samples, 0.0f), 1.0f) * 255.0f),
        static_cast<unsigned char>(fminf(fmaxf(accumulated.z * inverse_samples, 0.0f), 1.0f) * 255.0f), 255);
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
    if (optixGetPayload_3() == 0u) {
        const uint3 launch_index = optixGetLaunchIndex();
        const unsigned int pixel = launch_index.y * params.width + launch_index.x;
        params.gbuffer_normal[pixel] = normal;
        params.gbuffer_depth[pixel] = distance;
        params.gbuffer_albedo[pixel] = base_color;
    }
    const float reflectivity = params.primitive_reflectivity[primitive];
    const float roughness = params.primitive_roughness[primitive];
    float3 reflection_color = make_float3(0.0f, 0.0f, 0.0f);
    const unsigned int depth = optixGetPayload_3();
    if (reflectivity > 0.001f && depth < 2) {
        const uint3 launch_index = optixGetLaunchIndex();
        unsigned int random_state = hash(launch_index.x + params.width * launch_index.y +
                                         0x9e3779b9u * (depth + 1u) +
                                         0x85ebca6bu * (params.sample_index + 1u));
        float3 half_vector = sample_ggx_half_vector(normal, roughness, random_state);
        if (incoming.x * half_vector.x + incoming.y * half_vector.y + incoming.z * half_vector.z > 0.0f)
            half_vector = make_float3(-half_vector.x, -half_vector.y, -half_vector.z);
        const float incoming_dot_half = incoming.x * half_vector.x + incoming.y * half_vector.y +
                                        incoming.z * half_vector.z;
        float3 reflection_direction = make_float3(
            incoming.x - 2.0f * incoming_dot_half * half_vector.x,
            incoming.y - 2.0f * incoming_dot_half * half_vector.y,
            incoming.z - 2.0f * incoming_dot_half * half_vector.z);
        if (reflection_direction.x * normal.x + reflection_direction.y * normal.y +
                reflection_direction.z * normal.z <= 0.0f) {
            const float incoming_dot_normal = incoming.x * normal.x + incoming.y * normal.y +
                                              incoming.z * normal.z;
            reflection_direction = make_float3(
                incoming.x - 2.0f * incoming_dot_normal * normal.x,
                incoming.y - 2.0f * incoming_dot_normal * normal.y,
                incoming.z - 2.0f * incoming_dot_normal * normal.z);
        }
        unsigned int reflected_red = 0;
        unsigned int reflected_green = 0;
        unsigned int reflected_blue = 0;
        unsigned int reflected_depth = depth + 1;
        optixTrace(params.handle,
                   make_float3(hit.x + normal.x * 1.0e-3f, hit.y + normal.y * 1.0e-3f,
                               hit.z + normal.z * 1.0e-3f),
                   reflection_direction, 0.0f, 1.0e16f, 0.0f, OptixVisibilityMask(255),
                   OPTIX_RAY_FLAG_NONE, OPTIX_RAY_TYPE_RADIANCE, OPTIX_RAY_TYPE_COUNT,
                   OPTIX_RAY_TYPE_RADIANCE, reflected_red, reflected_green, reflected_blue,
                   reflected_depth);
        reflection_color = make_float3(__uint_as_float(reflected_red), __uint_as_float(reflected_green),
                                       __uint_as_float(reflected_blue));
    }
    const float local_weight = 1.0f - reflectivity;
    set_color(base_color.x * lighting * local_weight + reflection_color.x * reflectivity,
              base_color.y * lighting * local_weight + reflection_color.y * reflectivity,
              base_color.z * lighting * local_weight + reflection_color.z * reflectivity);
}
