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

static __forceinline__ __device__ void reservoir_update(OptixLightReservoir& reservoir,
                                                         unsigned int light_index,
                                                         float stream_weight,
                                                         float selected_weight,
                                                         unsigned int candidate_count,
                                                         unsigned int& random_state) {
    if (stream_weight <= 0.0f || candidate_count == 0u) return;
    reservoir.weight_sum += stream_weight;
    reservoir.candidate_count += candidate_count;
    if (random_float(random_state) * reservoir.weight_sum <= stream_weight) {
        reservoir.light_index = light_index;
        reservoir.selected_weight = selected_weight;
    }
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
    if (params.frame_index > 0u && params.sample_index == 16u) {
        const float2 motion = params.gbuffer_motion[pixel];
        const int previous_x = static_cast<int>(static_cast<float>(index.x) - motion.x + 0.5f);
        const int previous_y = static_cast<int>(static_cast<float>(index.y) - motion.y + 0.5f);
        bool valid = previous_x >= 0 && previous_x < static_cast<int>(params.width) &&
                     previous_y >= 0 && previous_y < static_cast<int>(params.height);
        if (valid) {
            const unsigned int previous_pixel = static_cast<unsigned int>(previous_y) * params.width +
                                                static_cast<unsigned int>(previous_x);
            const float current_depth = params.gbuffer_depth[pixel];
            const float previous_depth = params.previous_depth[previous_pixel];
            const float3 current_normal = params.gbuffer_normal[pixel];
            const float3 previous_normal = params.previous_normal[previous_pixel];
            const float3 current_albedo = params.gbuffer_albedo[pixel];
            const float3 previous_albedo = params.previous_albedo[previous_pixel];
            const float normal_similarity = current_normal.x * previous_normal.x +
                                            current_normal.y * previous_normal.y +
                                            current_normal.z * previous_normal.z;
            const float depth_threshold = fmaxf(1.0e-3f, current_depth * 0.01f);
            const float albedo_difference = fabsf(current_albedo.x - previous_albedo.x) +
                                            fabsf(current_albedo.y - previous_albedo.y) +
                                            fabsf(current_albedo.z - previous_albedo.z);
            valid = current_depth > 0.0f && previous_depth > 0.0f &&
                    fabsf(current_depth - previous_depth) <= depth_threshold &&
                    normal_similarity >= 0.95f && albedo_difference <= 0.05f;
        }
        params.temporal_validity[pixel] = valid ? 1u : 0u;
        if (!valid) params.accumulation[pixel] = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
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

extern "C" __global__ void __raygen__spatial_reuse() {
    const uint3 index = optixGetLaunchIndex();
    const unsigned int pixel = index.y * params.width + index.x;
    const float center_depth = params.gbuffer_depth[pixel];
    const float3 center_normal = params.gbuffer_normal[pixel];
    const float3 center_albedo = params.gbuffer_albedo[pixel];
    OptixLightReservoir result{};
    unsigned int random_state = hash(pixel + 0xa511e9b3u);
    const OptixLightReservoir center = params.reservoirs[pixel];
    reservoir_update(result, center.light_index, center.weight_sum, center.selected_weight,
                     center.candidate_count, random_state);

    const int2 offsets[4] = {make_int2(-1, 0), make_int2(1, 0), make_int2(0, -1), make_int2(0, 1)};
    for (unsigned int neighbor_index = 0; neighbor_index < 4; ++neighbor_index) {
        const int x = static_cast<int>(index.x) + offsets[neighbor_index].x;
        const int y = static_cast<int>(index.y) + offsets[neighbor_index].y;
        if (x < 0 || x >= static_cast<int>(params.width) || y < 0 || y >= static_cast<int>(params.height))
            continue;
        const unsigned int neighbor_pixel = static_cast<unsigned int>(y) * params.width +
                                            static_cast<unsigned int>(x);
        const float neighbor_depth = params.gbuffer_depth[neighbor_pixel];
        const float3 neighbor_normal = params.gbuffer_normal[neighbor_pixel];
        const float3 neighbor_albedo = params.gbuffer_albedo[neighbor_pixel];
        const float normal_similarity = center_normal.x * neighbor_normal.x +
                                        center_normal.y * neighbor_normal.y +
                                        center_normal.z * neighbor_normal.z;
        const float albedo_difference = fabsf(center_albedo.x - neighbor_albedo.x) +
                                        fabsf(center_albedo.y - neighbor_albedo.y) +
                                        fabsf(center_albedo.z - neighbor_albedo.z);
        const bool compatible = center_depth > 0.0f && neighbor_depth > 0.0f &&
            fabsf(center_depth - neighbor_depth) <= fmaxf(1.0e-3f, center_depth * 0.02f) &&
            normal_similarity >= 0.90f && albedo_difference <= 0.10f;
        if (!compatible) continue;
        const OptixLightReservoir neighbor = params.reservoirs[neighbor_pixel];
        reservoir_update(result, neighbor.light_index, neighbor.weight_sum, neighbor.selected_weight,
                         neighbor.candidate_count, random_state);
    }
    params.spatial_reservoirs[pixel] = result;
}

extern "C" __global__ void __raygen__spatial_resolve() {
    const uint3 index = optixGetLaunchIndex();
    const uint3 dimensions = optixGetLaunchDimensions();
    const float2 screen = make_float2(
        (2.0f * (static_cast<float>(index.x) + 0.5f) / static_cast<float>(dimensions.x) - 1.0f) *
            (static_cast<float>(dimensions.x) / static_cast<float>(dimensions.y)),
        2.0f * (static_cast<float>(index.y) + 0.5f) / static_cast<float>(dimensions.y) - 1.0f);
    const float3 raw_direction = make_float3(screen.x * params.view_scale,
                                             screen.y * params.view_scale,
                                             -2.5f * params.view_scale);
    const float3 direction = normalize_vector(raw_direction);
    unsigned int red = 0, green = 0, blue = 0, depth = 0;
    optixTrace(params.handle, params.camera_origin, direction, 0.0f, 1.0e16f, 0.0f,
               OptixVisibilityMask(255), OPTIX_RAY_FLAG_NONE,
               OPTIX_RAY_TYPE_RADIANCE, OPTIX_RAY_TYPE_COUNT,
               OPTIX_RAY_TYPE_RADIANCE, red, green, blue, depth);
    const unsigned int pixel = index.y * params.width + index.x;
    params.image[pixel] = make_uchar4(
        static_cast<unsigned char>(fminf(fmaxf(__uint_as_float(red), 0.0f), 1.0f) * 255.0f),
        static_cast<unsigned char>(fminf(fmaxf(__uint_as_float(green), 0.0f), 1.0f) * 255.0f),
        static_cast<unsigned char>(fminf(fmaxf(__uint_as_float(blue), 0.0f), 1.0f) * 255.0f), 255);
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
    const unsigned int depth = optixGetPayload_3();
    const uint3 launch_index = optixGetLaunchIndex();
    const unsigned int pixel = launch_index.y * params.width + launch_index.x;
    const float3 base_color = params.primitive_colors[primitive];
    bool temporal_valid = false;
    unsigned int temporal_pixel = pixel;
    if (depth == 0u && params.spatial_resolve == 0u) {
        params.gbuffer_normal[pixel] = normal;
        params.gbuffer_depth[pixel] = distance;
        params.gbuffer_albedo[pixel] = base_color;
        if (params.frame_index > 0u && params.sample_index == 16u) {
            const float2 motion = params.gbuffer_motion[pixel];
            const int previous_x = static_cast<int>(static_cast<float>(launch_index.x) - motion.x + 0.5f);
            const int previous_y = static_cast<int>(static_cast<float>(launch_index.y) - motion.y + 0.5f);
            if (previous_x >= 0 && previous_x < static_cast<int>(params.width) && previous_y >= 0 &&
                previous_y < static_cast<int>(params.height)) {
                temporal_pixel = static_cast<unsigned int>(previous_y) * params.width +
                                 static_cast<unsigned int>(previous_x);
                const float previous_depth = params.previous_depth[temporal_pixel];
                const float3 previous_normal = params.previous_normal[temporal_pixel];
                const float3 previous_albedo = params.previous_albedo[temporal_pixel];
                const float normal_similarity = normal.x * previous_normal.x + normal.y * previous_normal.y +
                                                normal.z * previous_normal.z;
                const float albedo_difference = fabsf(base_color.x - previous_albedo.x) +
                                                fabsf(base_color.y - previous_albedo.y) +
                                                fabsf(base_color.z - previous_albedo.z);
                temporal_valid = previous_depth > 0.0f &&
                                 fabsf(distance - previous_depth) <= fmaxf(1.0e-3f, distance * 0.01f) &&
                                 normal_similarity >= 0.95f && albedo_difference <= 0.05f;
            }
        }
    }

    unsigned int light_random = hash(pixel + 0x27d4eb2du * (params.sample_index + 1u));
    unsigned int selected_light = light_random % params.light_count;
    float reservoir_normalization = static_cast<float>(params.light_count);
    if (depth == 0u && params.spatial_resolve != 0u) {
        const OptixLightReservoir reservoir = params.spatial_reservoirs[pixel];
        selected_light = reservoir.light_index;
        reservoir_normalization = reservoir.candidate_count > 0u && reservoir.selected_weight > 0.0f
            ? reservoir.weight_sum /
                  (static_cast<float>(reservoir.candidate_count) * reservoir.selected_weight)
            : 0.0f;
    } else if (depth == 0u) {
        OptixLightReservoir reservoir = params.reservoirs[pixel];
        if (params.sample_index == 0u || params.sample_index == 16u) reservoir = {};
        if (params.sample_index == 16u && temporal_valid) {
            const OptixLightReservoir previous = params.previous_reservoirs[temporal_pixel];
            reservoir_update(reservoir, previous.light_index, previous.weight_sum,
                             previous.selected_weight, previous.candidate_count, light_random);
        }
        const unsigned int candidate = light_random % params.light_count;
        const OptixPointLight candidate_light = params.lights[candidate];
        const float3 candidate_delta = make_float3(candidate_light.position.x - hit.x,
                                                   candidate_light.position.y - hit.y,
                                                   candidate_light.position.z - hit.z);
        const float candidate_distance_squared = candidate_delta.x * candidate_delta.x +
                                                 candidate_delta.y * candidate_delta.y +
                                                 candidate_delta.z * candidate_delta.z;
        const float inverse_candidate_distance = rsqrtf(candidate_distance_squared);
        const float candidate_cosine = fmaxf(0.0f,
            normal.x * candidate_delta.x * inverse_candidate_distance +
            normal.y * candidate_delta.y * inverse_candidate_distance +
            normal.z * candidate_delta.z * inverse_candidate_distance);
        const float luminance = 0.2126f * candidate_light.intensity.x +
                                0.7152f * candidate_light.intensity.y +
                                0.0722f * candidate_light.intensity.z;
        const float target = luminance * candidate_cosine / fmaxf(candidate_distance_squared, 1.0e-4f);
        reservoir_update(reservoir, candidate, target * static_cast<float>(params.light_count),
                         target, 1u, light_random);
        params.reservoirs[pixel] = reservoir;
        selected_light = reservoir.light_index;
        reservoir_normalization = reservoir.candidate_count > 0u && reservoir.selected_weight > 0.0f
            ? reservoir.weight_sum /
                  (static_cast<float>(reservoir.candidate_count) * reservoir.selected_weight)
            : 0.0f;
    }

    const OptixPointLight light = params.lights[selected_light];
    const float3 to_light = make_float3(light.position.x - hit.x,
                                        light.position.y - hit.y,
                                        light.position.z - hit.z);
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
    const float inverse_distance_squared = 1.0f / fmaxf(light_distance * light_distance, 1.0e-4f);
    const float direct_scale = visible ? diffuse * inverse_distance_squared * reservoir_normalization : 0.0f;
    const float3 lighting = make_float3(0.08f + light.intensity.x * direct_scale,
                                        0.08f + light.intensity.y * direct_scale,
                                        0.08f + light.intensity.z * direct_scale);
    const float reflectivity = params.primitive_reflectivity[primitive];
    const float roughness = params.primitive_roughness[primitive];
    float3 reflection_color = make_float3(0.0f, 0.0f, 0.0f);
    if (reflectivity > 0.001f && depth < 2) {
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
    set_color(base_color.x * lighting.x * local_weight + reflection_color.x * reflectivity,
              base_color.y * lighting.y * local_weight + reflection_color.y * reflectivity,
              base_color.z * lighting.z * local_weight + reflection_color.z * reflectivity);
}
