#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "assets/gltf_loader.hpp"
#include "environment/environment_map.hpp"

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kInfinity = 1.0e30f;
constexpr int kTileWidth = 16;
constexpr int kTileHeight = 16;

void check(cudaError_t result, const char* operation) {
    if (result != cudaSuccess) throw std::runtime_error(std::string(operation) + ": " + cudaGetErrorString(result));
}

__host__ __device__ float3 add(float3 a, float3 b) { return make_float3(a.x + b.x, a.y + b.y, a.z + b.z); }
__host__ __device__ float3 sub(float3 a, float3 b) { return make_float3(a.x - b.x, a.y - b.y, a.z - b.z); }
__host__ __device__ float3 mul(float3 a, float value) { return make_float3(a.x * value, a.y * value, a.z * value); }
__host__ __device__ float3 mul(float3 a, float3 b) { return make_float3(a.x * b.x, a.y * b.y, a.z * b.z); }
__host__ __device__ float dot3(float3 a, float3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
__host__ __device__ float3 normalize3(float3 value) {
    const float inverse = rsqrtf(dot3(value, value));
    return mul(value, inverse);
}
__host__ __device__ float3 cross3(float3 a, float3 b) {
    return make_float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
__host__ __device__ float3 reflect3(float3 direction, float3 normal) {
    return sub(direction, mul(normal, 2.0f * dot3(direction, normal)));
}

struct Ray { float3 origin; float3 direction; };
struct Sphere {
    float3 center; float radius; float3 albedo; float metallic; float roughness;
    float transmission; float index_of_refraction;
};
struct Triangle {
    float3 first; float3 second; float3 third; float3 albedo; float metallic; float roughness;
    float transmission; float index_of_refraction;
    float3 first_normal; float3 second_normal; float3 third_normal; bool smooth;
    float2 first_uv; float2 second_uv; float2 third_uv; int texture_index{-1};
    float3 tangent; float tangent_handedness{1.0f}; int normal_texture_index{-1}; float normal_scale{1.0f};
    int metallic_roughness_texture_index{-1};
    float3 emissive; int emissive_texture_index{-1};
    int material_index{-1};
    float clearcoat{0.0f}; float clearcoat_roughness{0.04f}; int clearcoat_texture_index{-1};
    float3 sheen_color{}; float sheen_roughness{0.0f};
};
struct TextureDescriptor { int offset; int width; int height; };
struct ImportedAssets {
    std::vector<Triangle> triangles;
    std::vector<float3> texture_pixels;
    std::vector<TextureDescriptor> textures;
};
struct AreaLight { float3 center; float3 half_u; float3 half_v; float3 emission; };
struct LightSet { AreaLight lights[3]; int count; };
struct Camera { float3 origin; float3 target; float vertical_fov; };
struct GpuEnvironment {
    const float3* pixels;
    const float* pmf;
    const float* cdf;
    int width;
    int height;
};
struct GpuTextures { const float3* pixels; const TextureDescriptor* descriptors; int count; };
struct BvhNode { float3 minimum; float3 maximum; int left; int right; int first; int count; };
struct Hit {
    float distance; float3 position; float3 normal; float3 albedo; float metallic; float roughness;
    float transmission; float index_of_refraction; bool front_face; bool found;
    float2 uv; int texture_index;
    float3 tangent; float tangent_handedness; int normal_texture_index; float normal_scale;
    int metallic_roughness_texture_index;
    float3 emissive; int emissive_texture_index;
    float clearcoat; float clearcoat_roughness; int clearcoat_texture_index;
    float3 sheen_color; float sheen_roughness;
};

struct Rng {
    std::uint32_t state;
    __device__ explicit Rng(std::uint32_t seed) : state(seed ? seed : 1u) {}
    __device__ float next() {
        state ^= state << 13u;
        state ^= state >> 17u;
        state ^= state << 5u;
        return static_cast<float>(state) * 2.3283064365386963e-10f;
    }
};

__device__ bool intersect_box(const Ray& ray, const BvhNode& node, float maximum_distance) {
    float near_distance = 1.0e-4f;
    float far_distance = maximum_distance;
    const float origins[3]{ray.origin.x, ray.origin.y, ray.origin.z};
    const float directions[3]{ray.direction.x, ray.direction.y, ray.direction.z};
    const float minimums[3]{node.minimum.x, node.minimum.y, node.minimum.z};
    const float maximums[3]{node.maximum.x, node.maximum.y, node.maximum.z};
    for (int axis = 0; axis < 3; ++axis) {
        if (fabsf(directions[axis]) < 1.0e-12f) {
            if (origins[axis] < minimums[axis] || origins[axis] > maximums[axis]) return false;
            continue;
        }
        const float inverse = 1.0f / directions[axis];
        float first = (minimums[axis] - origins[axis]) * inverse;
        float second = (maximums[axis] - origins[axis]) * inverse;
        if (inverse < 0.0f) { const float swap = first; first = second; second = swap; }
        near_distance = fmaxf(near_distance, first);
        far_distance = fminf(far_distance, second);
        if (far_distance < near_distance) return false;
    }
    return true;
}

__device__ void intersect_triangles(const Ray& ray, const Triangle* triangles, const BvhNode* nodes,
                                    int node_count, Hit& closest) {
    if (node_count == 0) return;
    int stack[64];
    int stack_size = 1;
    stack[0] = 0;
    while (stack_size > 0) {
        const BvhNode node = nodes[stack[--stack_size]];
        if (!intersect_box(ray, node, closest.distance)) continue;
        if (node.count > 0) {
            for (int index = node.first; index < node.first + node.count; ++index) {
                const Triangle triangle = triangles[index];
                const float3 edge1 = sub(triangle.second, triangle.first);
                const float3 edge2 = sub(triangle.third, triangle.first);
                const float3 p = cross3(ray.direction, edge2);
                const float determinant = dot3(edge1, p);
                if (fabsf(determinant) < 1.0e-8f) continue;
                const float inverse = 1.0f / determinant;
                const float3 offset = sub(ray.origin, triangle.first);
                const float u = dot3(offset, p) * inverse;
                if (u < 0.0f || u > 1.0f) continue;
                const float3 q = cross3(offset, edge1);
                const float v = dot3(ray.direction, q) * inverse;
                if (v < 0.0f || u + v > 1.0f) continue;
                const float distance = dot3(edge2, q) * inverse;
                if (distance <= 1.0e-4f || distance >= closest.distance) continue;
                closest.distance = distance;
                closest.position = add(ray.origin, mul(ray.direction, distance));
                const float3 geometric_normal = normalize3(cross3(edge1, edge2));
                closest.normal = triangle.smooth
                    ? normalize3(add(mul(triangle.first_normal, 1.0f - u - v),
                                     add(mul(triangle.second_normal, u), mul(triangle.third_normal, v))))
                    : geometric_normal;
                closest.front_face = dot3(geometric_normal, ray.direction) < 0.0f;
                if (dot3(closest.normal, ray.direction) >= 0.0f)
                    closest.normal = mul(closest.normal, -1.0f);
                closest.albedo = triangle.albedo;
                closest.metallic = triangle.metallic;
                closest.roughness = triangle.roughness;
                closest.transmission = triangle.transmission;
                closest.index_of_refraction = triangle.index_of_refraction;
                closest.uv = make_float2(triangle.first_uv.x * (1.0f - u - v) + triangle.second_uv.x * u +
                                         triangle.third_uv.x * v,
                                         triangle.first_uv.y * (1.0f - u - v) + triangle.second_uv.y * u +
                                         triangle.third_uv.y * v);
                closest.texture_index = triangle.texture_index;
                closest.tangent = triangle.tangent;
                closest.tangent_handedness = triangle.tangent_handedness;
                closest.normal_texture_index = triangle.normal_texture_index;
                closest.normal_scale = triangle.normal_scale;
                closest.metallic_roughness_texture_index = triangle.metallic_roughness_texture_index;
                closest.emissive = triangle.emissive;
                closest.emissive_texture_index = triangle.emissive_texture_index;
                closest.clearcoat = triangle.clearcoat;
                closest.clearcoat_roughness = triangle.clearcoat_roughness;
                closest.clearcoat_texture_index = triangle.clearcoat_texture_index;
                closest.sheen_color = triangle.sheen_color;
                closest.sheen_roughness = triangle.sheen_roughness;
                closest.found = true;
            }
        } else {
            if (stack_size + 2 > 64) return;
            stack[stack_size++] = node.left;
            stack[stack_size++] = node.right;
        }
    }
}

__device__ bool intersect_scene(const Ray& ray, const Sphere* spheres, int sphere_count,
                                const Triangle* triangles, const BvhNode* nodes, int node_count,
                                Hit& closest) {
    closest.distance = kInfinity;
    closest.found = false;
    for (int index = 0; index < sphere_count; ++index) {
        const Sphere sphere = spheres[index];
        const float3 offset = sub(ray.origin, sphere.center);
        const float half_b = dot3(offset, ray.direction);
        const float c = dot3(offset, offset) - sphere.radius * sphere.radius;
        const float discriminant = half_b * half_b - c;
        if (discriminant < 0.0f) continue;
        const float root = sqrtf(discriminant);
        float distance = -half_b - root;
        if (distance <= 1.0e-4f) distance = -half_b + root;
        if (distance <= 1.0e-4f || distance >= closest.distance) continue;
        closest.distance = distance;
        closest.position = add(ray.origin, mul(ray.direction, distance));
        closest.normal = normalize3(sub(closest.position, sphere.center));
        closest.front_face = dot3(closest.normal, ray.direction) < 0.0f;
        if (!closest.front_face) closest.normal = mul(closest.normal, -1.0f);
        closest.albedo = sphere.albedo;
        closest.metallic = sphere.metallic;
        closest.roughness = sphere.roughness;
        closest.transmission = sphere.transmission;
        closest.index_of_refraction = sphere.index_of_refraction;
        closest.texture_index = -1;
        closest.normal_texture_index = -1;
        closest.metallic_roughness_texture_index = -1;
        closest.emissive = make_float3(0, 0, 0);
        closest.emissive_texture_index = -1;
        closest.clearcoat = 0.0f;
        closest.clearcoat_texture_index = -1;
        closest.sheen_color = make_float3(0, 0, 0);
        closest.sheen_roughness = 0.0f;
        closest.found = true;
    }
    intersect_triangles(ray, triangles, nodes, node_count, closest);
    return closest.found;
}

__device__ float3 cosine_direction(float3 normal, Rng& rng) {
    const float radius = sqrtf(rng.next());
    const float angle = 2.0f * kPi * rng.next();
    const float x = radius * cosf(angle);
    const float y = radius * sinf(angle);
    const float z = sqrtf(fmaxf(0.0f, 1.0f - radius * radius));
    const float3 helper = fabsf(normal.x) > 0.9f ? make_float3(0, 1, 0) : make_float3(1, 0, 0);
    const float3 tangent = normalize3(cross3(helper, normal));
    const float3 bitangent = cross3(normal, tangent);
    return normalize3(add(add(mul(tangent, x), mul(bitangent, y)), mul(normal, z)));
}

__device__ float3 sample_texture(const GpuTextures& textures, int index, float2 uv) {
    if (index < 0 || index >= textures.count) return make_float3(1, 1, 1);
    const TextureDescriptor texture = textures.descriptors[index];
    uv.x -= floorf(uv.x); uv.y -= floorf(uv.y);
    const float x = uv.x * texture.width - 0.5f, y = uv.y * texture.height - 0.5f;
    const int x0 = static_cast<int>(floorf(x)), y0 = static_cast<int>(floorf(y));
    const auto texel = [&](int column, int row) {
        column = (column % texture.width + texture.width) % texture.width;
        row = (row % texture.height + texture.height) % texture.height;
        return textures.pixels[texture.offset + row * texture.width + column];
    };
    const float tx = x - floorf(x), ty = y - floorf(y);
    return add(mul(add(mul(texel(x0, y0), 1.0f - tx), mul(texel(x0 + 1, y0), tx)), 1.0f - ty),
               mul(add(mul(texel(x0, y0 + 1), 1.0f - tx), mul(texel(x0 + 1, y0 + 1), tx)), ty));
}

__device__ float3 sample_ggx_normal(float3 normal, float roughness, Rng& rng) {
    const float alpha = fmaxf(0.001f, roughness * roughness);
    const float uniform = fminf(rng.next(), 1.0f - 1.0e-7f);
    const float tangent_squared = alpha * alpha * uniform / (1.0f - uniform);
    const float cosine = rsqrtf(1.0f + tangent_squared);
    const float sine = sqrtf(fmaxf(0.0f, 1.0f - cosine * cosine));
    const float azimuth = 2.0f * kPi * rng.next();
    const float3 helper = fabsf(normal.x) > 0.9f ? make_float3(0, 1, 0) : make_float3(1, 0, 0);
    const float3 tangent = normalize3(cross3(helper, normal));
    const float3 bitangent = cross3(normal, tangent);
    return normalize3(add(add(mul(tangent, sine * cosf(azimuth)),
                              mul(bitangent, sine * sinf(azimuth))), mul(normal, cosine)));
}

__host__ __device__ float ggx_distribution(float normal_dot_half, float roughness) {
    const float alpha = fmaxf(0.001f, roughness * roughness);
    const float alpha_squared = alpha * alpha;
    const float cosine_squared = fmaxf(0.0f, normal_dot_half) * fmaxf(0.0f, normal_dot_half);
    const float denominator = cosine_squared * (alpha_squared - 1.0f) + 1.0f;
    return alpha_squared / (kPi * denominator * denominator);
}

__host__ __device__ float smith_schlick(float normal_dot_direction, float roughness) {
    const float r = roughness + 1.0f;
    const float k = r * r / 8.0f;
    return normal_dot_direction / (normal_dot_direction * (1.0f - k) + k);
}

__host__ __device__ float3 fresnel_schlick(float cosine, float3 reflectance_at_normal) {
    const float factor = powf(1.0f - fminf(1.0f, fmaxf(0.0f, cosine)), 5.0f);
    return add(reflectance_at_normal, mul(sub(make_float3(1, 1, 1), reflectance_at_normal), factor));
}

__device__ float3 evaluate_ggx_metal(float3 normal, float3 view, float3 light,
                                     float3 base_color, float roughness) {
    const float n_dot_v = fmaxf(0.0f, dot3(normal, view));
    const float n_dot_l = fmaxf(0.0f, dot3(normal, light));
    if (n_dot_v <= 0.0f || n_dot_l <= 0.0f) return make_float3(0, 0, 0);
    const float3 half_vector = normalize3(add(view, light));
    const float n_dot_h = fmaxf(0.0f, dot3(normal, half_vector));
    const float v_dot_h = fmaxf(0.0f, dot3(view, half_vector));
    const float distribution = ggx_distribution(n_dot_h, roughness);
    const float geometry = smith_schlick(n_dot_v, roughness) * smith_schlick(n_dot_l, roughness);
    return mul(fresnel_schlick(v_dot_h, base_color),
               distribution * geometry / fmaxf(4.0f * n_dot_v * n_dot_l, 1.0e-8f));
}

__device__ float ggx_reflection_pdf(float3 normal, float3 view, float3 light, float roughness) {
    if (dot3(normal, view) <= 0.0f || dot3(normal, light) <= 0.0f) return 0.0f;
    const float3 half_vector = normalize3(add(view, light));
    const float n_dot_h = fmaxf(0.0f, dot3(normal, half_vector));
    const float v_dot_h = fabsf(dot3(view, half_vector));
    return v_dot_h > 0.0f ? ggx_distribution(n_dot_h, roughness) * n_dot_h / (4.0f * v_dot_h) : 0.0f;
}

__device__ float specular_probability(float metallic) {
    return 0.5f + 0.5f * fminf(1.0f, fmaxf(0.0f, metallic));
}

__device__ float clearcoat_probability(float clearcoat) {
    return 0.25f * fminf(1.0f, fmaxf(0.0f, clearcoat));
}

__device__ float3 evaluate_metallic_roughness(float3 normal, float3 view, float3 light,
                                               float3 base_color, float metallic, float roughness,
                                               float clearcoat = 0.0f, float clearcoat_roughness = 0.04f,
                                               float3 sheen_color = {}, float sheen_roughness = 0.0f) {
    const float n_dot_v = fmaxf(0.0f, dot3(normal, view));
    const float n_dot_l = fmaxf(0.0f, dot3(normal, light));
    if (n_dot_v <= 0.0f || n_dot_l <= 0.0f) return make_float3(0, 0, 0);
    const float3 half_vector = normalize3(add(view, light));
    const float v_dot_h = fmaxf(0.0f, dot3(view, half_vector));
    const float3 f0 = add(mul(make_float3(0.04f, 0.04f, 0.04f), 1.0f - metallic),
                          mul(base_color, metallic));
    const float3 fresnel = fresnel_schlick(v_dot_h, f0);
    const float distribution = ggx_distribution(fmaxf(0.0f, dot3(normal, half_vector)), roughness);
    const float geometry = smith_schlick(n_dot_v, roughness) * smith_schlick(n_dot_l, roughness);
    const float3 specular = mul(fresnel,
        distribution * geometry / fmaxf(4.0f * n_dot_v * n_dot_l, 1.0e-8f));
    const float3 diffuse_weight = mul(sub(make_float3(1, 1, 1), fresnel), 1.0f - metallic);
    float3 base = add(specular, mul(mul(diffuse_weight, base_color), 1.0f / kPi));
    const float sheen_strength = fmaxf(sheen_color.x, fmaxf(sheen_color.y, sheen_color.z));
    if (sheen_strength > 0.0f) {
        const float n_dot_h = fmaxf(0.0f, dot3(normal, half_vector));
        const float alpha = fmaxf(0.001f, sheen_roughness * sheen_roughness);
        const float inverse_alpha = 1.0f / alpha;
        const float sin_theta_h = sqrtf(fmaxf(0.0f, 1.0f - n_dot_h * n_dot_h));
        const float distribution_charlie = (2.0f + inverse_alpha) / (2.0f * kPi) *
                                           powf(sin_theta_h, inverse_alpha);
        const float visibility_neubelt = 1.0f /
            fmaxf(4.0f * (n_dot_l + n_dot_v - n_dot_l * n_dot_v), 1.0e-5f);
        base = add(mul(base, 1.0f - 0.157f * sheen_strength),
                   mul(sheen_color, distribution_charlie * visibility_neubelt));
    }
    const float coat = fminf(1.0f, fmaxf(0.0f, clearcoat));
    if (coat <= 0.0f) return base;
    const float3 coat_fresnel = fresnel_schlick(v_dot_h, make_float3(0.04f, 0.04f, 0.04f));
    const float coat_distribution = ggx_distribution(
        fmaxf(0.0f, dot3(normal, half_vector)), clearcoat_roughness);
    const float coat_geometry = smith_schlick(n_dot_v, clearcoat_roughness) *
                                smith_schlick(n_dot_l, clearcoat_roughness);
    const float3 coat_specular = mul(coat_fresnel, coat * coat_distribution * coat_geometry /
        fmaxf(4.0f * n_dot_v * n_dot_l, 1.0e-8f));
    return add(mul(base, 1.0f - coat * coat_fresnel.x), coat_specular);
}

__device__ float metallic_roughness_pdf(float3 normal, float3 view, float3 light,
                                         float metallic, float roughness,
                                         float clearcoat = 0.0f, float clearcoat_roughness = 0.04f) {
    const float coat = clearcoat_probability(clearcoat);
    const float base_specular = (1.0f - coat) * specular_probability(metallic);
    const float diffuse = 1.0f - coat - base_specular;
    return coat * ggx_reflection_pdf(normal, view, light, clearcoat_roughness) +
           base_specular * ggx_reflection_pdf(normal, view, light, roughness) +
           diffuse * fmaxf(0.0f, dot3(normal, light)) / kPi;
}

__device__ float fresnel_dielectric(float cosine, float eta_incident, float eta_transmitted) {
    cosine = fminf(1.0f, fabsf(cosine));
    const float sine_transmitted = eta_incident / eta_transmitted *
                                   sqrtf(fmaxf(0.0f, 1.0f - cosine * cosine));
    if (sine_transmitted >= 1.0f) return 1.0f;
    const float cosine_transmitted = sqrtf(fmaxf(0.0f, 1.0f - sine_transmitted * sine_transmitted));
    const float parallel = (eta_transmitted * cosine - eta_incident * cosine_transmitted) /
                           (eta_transmitted * cosine + eta_incident * cosine_transmitted);
    const float perpendicular = (eta_incident * cosine - eta_transmitted * cosine_transmitted) /
                                (eta_incident * cosine + eta_transmitted * cosine_transmitted);
    return 0.5f * (parallel * parallel + perpendicular * perpendicular);
}

__device__ bool refract3(float3 incident, float3 normal, float eta_ratio, float3& transmitted) {
    const float cosine = fminf(1.0f, -dot3(incident, normal));
    const float discriminant = 1.0f - eta_ratio * eta_ratio * (1.0f - cosine * cosine);
    if (discriminant < 0.0f) return false;
    transmitted = normalize3(add(mul(incident, eta_ratio),
                                 mul(normal, eta_ratio * cosine - sqrtf(discriminant))));
    return true;
}

__device__ float dielectric_reflection_pdf(float3 normal, float3 view, float3 light,
                                           float3 half_vector, float roughness, float fresnel) {
    const float n_dot_h = fmaxf(0.0f, dot3(normal, half_vector));
    const float v_dot_h = fabsf(dot3(view, half_vector));
    return v_dot_h > 0.0f
        ? fresnel * ggx_distribution(n_dot_h, roughness) * n_dot_h / (4.0f * v_dot_h)
        : 0.0f;
}

__device__ float dielectric_transmission_pdf(float3 normal, float3 view, float3 transmitted,
                                             float3 half_vector, float roughness, float fresnel,
                                             float eta_incident, float eta_transmitted) {
    const float n_dot_h = fmaxf(0.0f, dot3(normal, half_vector));
    const float denominator = eta_incident * dot3(view, half_vector) +
                              eta_transmitted * dot3(transmitted, half_vector);
    if (fabsf(denominator) <= 1.0e-8f) return 0.0f;
    const float jacobian = eta_transmitted * eta_transmitted *
                           fabsf(dot3(transmitted, half_vector)) / (denominator * denominator);
    return (1.0f - fresnel) * ggx_distribution(n_dot_h, roughness) * n_dot_h * jacobian;
}

__device__ float3 evaluate_dielectric_btdf(float3 normal, float3 view, float3 transmitted,
                                           float3 half_vector, float3 tint, float roughness,
                                           float fresnel, float eta_incident, float eta_transmitted) {
    const float n_dot_v = fabsf(dot3(normal, view));
    const float n_dot_l = fabsf(dot3(normal, transmitted));
    const float n_dot_h = fmaxf(0.0f, dot3(normal, half_vector));
    const float v_dot_h = dot3(view, half_vector);
    const float l_dot_h = dot3(transmitted, half_vector);
    const float denominator = eta_incident * v_dot_h + eta_transmitted * l_dot_h;
    if (n_dot_v <= 0.0f || n_dot_l <= 0.0f || fabsf(denominator) <= 1.0e-8f)
        return make_float3(0, 0, 0);
    const float distribution = ggx_distribution(n_dot_h, roughness);
    const float geometry = smith_schlick(n_dot_v, roughness) * smith_schlick(n_dot_l, roughness);
    const float factor = (1.0f - fresnel) * distribution * geometry * eta_transmitted * eta_transmitted *
                         fabsf(v_dot_h * l_dot_h) /
                         (n_dot_v * n_dot_l * denominator * denominator);
    return mul(tint, factor);
}

__host__ __device__ float power_heuristic(float first_pdf, float second_pdf) {
    const float first = first_pdf * first_pdf;
    const float second = second_pdf * second_pdf;
    return first / (first + second);
}

__host__ __device__ float light_area(const AreaLight& light) {
    return 4.0f * sqrtf(dot3(cross3(light.half_u, light.half_v),
                            cross3(light.half_u, light.half_v)));
}

__device__ bool intersect_light(const AreaLight& light, const Ray& ray, float& distance) {
    const float3 normal = normalize3(cross3(light.half_u, light.half_v));
    const float denominator = dot3(normal, ray.direction);
    if (fabsf(denominator) < 1.0e-8f) return false;
    distance = dot3(sub(light.center, ray.origin), normal) / denominator;
    if (distance <= 1.0e-4f) return false;
    const float3 relative = sub(add(ray.origin, mul(ray.direction, distance)), light.center);
    const float u = dot3(relative, light.half_u) / dot3(light.half_u, light.half_u);
    const float v = dot3(relative, light.half_v) / dot3(light.half_v, light.half_v);
    return fabsf(u) <= 1.0f && fabsf(v) <= 1.0f;
}

__device__ float area_light_pdf(const AreaLight& light, const Ray& ray, float distance) {
    const float3 normal = normalize3(cross3(light.half_u, light.half_v));
    const float cosine = fabsf(dot3(normal, ray.direction));
    return cosine > 0.0f ? distance * distance / (cosine * light_area(light)) : 0.0f;
}

__device__ float3 environment_texel(const GpuEnvironment& environment, int x, int y) {
    x = (x % environment.width + environment.width) % environment.width;
    y = max(0, min(environment.height - 1, y));
    return environment.pixels[y * environment.width + x];
}

__device__ float3 sample_environment(const GpuEnvironment& environment, float3 direction);

__device__ int environment_index(const GpuEnvironment& environment, float3 direction) {
    direction = normalize3(direction);
    const float u = atan2f(direction.z, direction.x) / (2.0f * kPi) + 0.5f;
    const float v = acosf(fminf(1.0f, fmaxf(-1.0f, direction.y))) / kPi;
    const int x = min(environment.width - 1, max(0, static_cast<int>(u * environment.width)));
    const int y = min(environment.height - 1, max(0, static_cast<int>(v * environment.height)));
    return y * environment.width + x;
}

__device__ float environment_texel_solid_angle(const GpuEnvironment& environment, int index) {
    const int y = index / environment.width;
    const float theta0 = kPi * static_cast<float>(y) / static_cast<float>(environment.height);
    const float theta1 = kPi * static_cast<float>(y + 1) / static_cast<float>(environment.height);
    return 2.0f * kPi / static_cast<float>(environment.width) * (cosf(theta0) - cosf(theta1));
}

__device__ float environment_pdf(const GpuEnvironment& environment, float3 direction) {
    if (!environment.pmf) return 0.0f;
    const int index = environment_index(environment, direction);
    return environment.pmf[index] / environment_texel_solid_angle(environment, index);
}

__device__ bool sample_environment_direction(const GpuEnvironment& environment, Rng& rng,
                                             float3& direction, float3& value, float& pdf) {
    if (!environment.cdf || !environment.pmf) return false;
    const int count = environment.width * environment.height;
    const float target = rng.next();
    int low = 0;
    int high = count - 1;
    while (low < high) {
        const int middle = low + (high - low) / 2;
        if (target <= environment.cdf[middle]) high = middle;
        else low = middle + 1;
    }
    const int index = low;
    const int x = index % environment.width;
    const int y = index / environment.width;
    const float u = (static_cast<float>(x) + rng.next()) / static_cast<float>(environment.width);
    const float v = (static_cast<float>(y) + rng.next()) / static_cast<float>(environment.height);
    const float theta = kPi * v;
    const float phi = 2.0f * kPi * (u - 0.5f);
    const float sine = sinf(theta);
    direction = make_float3(sine * cosf(phi), cosf(theta), sine * sinf(phi));
    pdf = environment.pmf[index] / environment_texel_solid_angle(environment, index);
    value = sample_environment(environment, direction);
    return pdf > 0.0f;
}

__device__ float3 sample_environment(const GpuEnvironment& environment, float3 direction) {
    if (!environment.pixels || environment.width <= 0 || environment.height <= 0) {
        const float blend = 0.5f * (direction.y + 1.0f);
        return add(mul(make_float3(0.02f, 0.03f, 0.06f), 1.0f - blend),
                   mul(make_float3(0.35f, 0.55f, 0.9f), blend));
    }
    direction = normalize3(direction);
    const float u = atan2f(direction.z, direction.x) / (2.0f * kPi) + 0.5f;
    const float v = acosf(fminf(1.0f, fmaxf(-1.0f, direction.y))) / kPi;
    const float x = u * environment.width - 0.5f;
    const float y = v * environment.height - 0.5f;
    const int x0 = static_cast<int>(floorf(x));
    const int y0 = static_cast<int>(floorf(y));
    const float tx = x - floorf(x);
    const float ty = y - floorf(y);
    const float3 top = add(mul(environment_texel(environment, x0, y0), 1.0f - tx),
                           mul(environment_texel(environment, x0 + 1, y0), tx));
    const float3 bottom = add(mul(environment_texel(environment, x0, y0 + 1), 1.0f - tx),
                              mul(environment_texel(environment, x0 + 1, y0 + 1), tx));
    return add(mul(top, 1.0f - ty), mul(bottom, ty));
}

__device__ float3 radiance(Ray ray, const Sphere* spheres, int sphere_count,
                           const Triangle* triangles, const BvhNode* nodes, int node_count,
                           const LightSet& lights, const GpuEnvironment& environment,
                           const GpuTextures& textures, Rng& rng) {
    float3 result = make_float3(0, 0, 0);
    float3 throughput = make_float3(1, 1, 1);
    float previous_bsdf_pdf = 0.0f;
    bool previous_uses_mis = false;
    for (int depth = 0; depth < 6; ++depth) {
        Hit hit{};
        const bool hit_scene = intersect_scene(ray, spheres, sphere_count, triangles, nodes, node_count, hit);
        float light_distance = kInfinity;
        int hit_light_index = -1;
        for (int index = 0; index < lights.count; ++index) {
            float candidate_distance = 0.0f;
            if (intersect_light(lights.lights[index], ray, candidate_distance) &&
                candidate_distance < light_distance) {
                light_distance = candidate_distance;
                hit_light_index = index;
            }
        }
        const bool hit_light = hit_light_index >= 0;
        if (hit_light && (!hit_scene || light_distance < hit.distance)) {
            const AreaLight light = lights.lights[hit_light_index];
            float weight = 1.0f;
            if (previous_uses_mis)
                weight = power_heuristic(previous_bsdf_pdf,
                    area_light_pdf(light, ray, light_distance) / static_cast<float>(lights.count));
            result = add(result, mul(mul(throughput, light.emission), weight));
            break;
        }
        if (!hit_scene) {
            float weight = 1.0f;
            if (previous_uses_mis && environment.pmf)
                weight = power_heuristic(previous_bsdf_pdf, environment_pdf(environment, ray.direction));
            result = add(result, mul(mul(throughput, sample_environment(environment, ray.direction)), weight));
            break;
        }
        hit.albedo = mul(hit.albedo, sample_texture(textures, hit.texture_index, hit.uv));
        if (hit.emissive_texture_index >= 0)
            hit.emissive = mul(hit.emissive, sample_texture(textures, hit.emissive_texture_index, hit.uv));
        result = add(result, mul(throughput, hit.emissive));
        if (hit.clearcoat_texture_index >= 0)
            hit.clearcoat *= sample_texture(textures, hit.clearcoat_texture_index, hit.uv).x;
        if (hit.metallic_roughness_texture_index >= 0) {
            const float3 packed = sample_texture(textures, hit.metallic_roughness_texture_index, hit.uv);
            hit.roughness = fminf(1.0f, fmaxf(0.04f, hit.roughness * packed.y));
            hit.metallic = fminf(1.0f, fmaxf(0.0f, hit.metallic * packed.z));
        }
        if (hit.normal_texture_index >= 0) {
            const float3 encoded = sample_texture(textures, hit.normal_texture_index, hit.uv);
            const float3 tangent_normal = normalize3(make_float3(
                (encoded.x * 2.0f - 1.0f) * hit.normal_scale,
                (encoded.y * 2.0f - 1.0f) * hit.normal_scale,
                encoded.z * 2.0f - 1.0f));
            const float3 tangent = normalize3(sub(hit.tangent, mul(hit.normal, dot3(hit.normal, hit.tangent))));
            const float3 bitangent = mul(cross3(hit.normal, tangent), hit.tangent_handedness);
            const float3 mapped = normalize3(add(add(mul(tangent, tangent_normal.x),
                                                      mul(bitangent, tangent_normal.y)),
                                                  mul(hit.normal, tangent_normal.z)));
            hit.normal = dot3(mapped, ray.direction) < 0.0f ? mapped : mul(mapped, -1.0f);
        }
        const float3 view_direction = mul(ray.direction, -1.0f);
        const bool non_transmissive_surface = hit.transmission <= 0.0f;
        if (non_transmissive_surface) {
            const int light_index = min(static_cast<int>(rng.next() * lights.count), lights.count - 1);
            const AreaLight light = lights.lights[light_index];
            const float3 light_point = add(light.center, add(mul(light.half_u, 2.0f * rng.next() - 1.0f),
                                                               mul(light.half_v, 2.0f * rng.next() - 1.0f)));
            const float3 offset = sub(light_point, hit.position);
            const float distance_squared = dot3(offset, offset);
            const float distance = sqrtf(distance_squared);
            const float3 direction_to_light = mul(offset, 1.0f / distance);
            const float surface_cosine = fmaxf(0.0f, dot3(hit.normal, direction_to_light));
            const float3 light_normal = normalize3(cross3(light.half_u, light.half_v));
            const float light_cosine = fmaxf(0.0f, -dot3(light_normal, direction_to_light));
            if (surface_cosine > 0.0f && light_cosine > 0.0f) {
                Hit blocker{};
                const Ray shadow{add(hit.position, mul(hit.normal, 1.0e-4f)), direction_to_light};
                const bool blocked = intersect_scene(shadow, spheres, sphere_count, triangles, nodes,
                                                     node_count, blocker) && blocker.distance < distance - 1.0e-4f;
                if (!blocked) {
                    const float light_pdf = distance_squared /
                        (light_cosine * light_area(light) * static_cast<float>(lights.count));
                    const float3 brdf = evaluate_metallic_roughness(
                        hit.normal, view_direction, direction_to_light,
                        hit.albedo, hit.metallic, hit.roughness,
                        hit.clearcoat, hit.clearcoat_roughness, hit.sheen_color, hit.sheen_roughness);
                    const float bsdf_pdf = metallic_roughness_pdf(
                        hit.normal, view_direction, direction_to_light, hit.metallic, hit.roughness,
                        hit.clearcoat, hit.clearcoat_roughness);
                    const float weight = power_heuristic(light_pdf, bsdf_pdf);
                    result = add(result, mul(mul(mul(throughput, brdf), light.emission),
                                             surface_cosine * weight / light_pdf));
                }
            }
            float3 environment_direction{};
            float3 environment_value{};
            float environment_sample_pdf = 0.0f;
            if (sample_environment_direction(environment, rng, environment_direction,
                                             environment_value, environment_sample_pdf)) {
                const float environment_cosine = fmaxf(0.0f, dot3(hit.normal, environment_direction));
                if (environment_cosine > 0.0f) {
                    Hit blocker{};
                    const Ray shadow{add(hit.position, mul(hit.normal, 1.0e-4f)), environment_direction};
                    const bool blocked = intersect_scene(shadow, spheres, sphere_count, triangles, nodes,
                                                         node_count, blocker);
                    if (!blocked) {
                        const float3 brdf = evaluate_metallic_roughness(
                            hit.normal, view_direction, environment_direction,
                            hit.albedo, hit.metallic, hit.roughness,
                            hit.clearcoat, hit.clearcoat_roughness, hit.sheen_color, hit.sheen_roughness);
                        const float bsdf_pdf = metallic_roughness_pdf(
                            hit.normal, view_direction, environment_direction, hit.metallic, hit.roughness,
                            hit.clearcoat, hit.clearcoat_roughness);
                        const float weight = power_heuristic(environment_sample_pdf, bsdf_pdf);
                        result = add(result, mul(mul(mul(throughput, brdf), environment_value),
                                                 environment_cosine * weight / environment_sample_pdf));
                    }
                }
            }
        }
        float3 direction;
        if (hit.transmission > 0.0f && rng.next() < hit.transmission) {
            float3 microfacet = sample_ggx_normal(hit.normal, hit.roughness, rng);
            if (dot3(ray.direction, microfacet) > 0.0f) microfacet = mul(microfacet, -1.0f);
            const float eta_incident = hit.front_face ? 1.0f : hit.index_of_refraction;
            const float eta_transmitted = hit.front_face ? hit.index_of_refraction : 1.0f;
            const float fresnel = fresnel_dielectric(-dot3(ray.direction, microfacet),
                                                      eta_incident, eta_transmitted);
            float3 transmitted{};
            const bool can_refract = refract3(ray.direction, microfacet,
                                               eta_incident / eta_transmitted, transmitted);
            if (!can_refract || rng.next() < fresnel) {
                direction = normalize3(reflect3(ray.direction, microfacet));
                const float n_dot_v = fmaxf(0.0f, dot3(hit.normal, view_direction));
                const float n_dot_l = fmaxf(0.0f, dot3(hit.normal, direction));
                const float n_dot_h = fmaxf(0.0f, dot3(hit.normal, microfacet));
                const float v_dot_h = fabsf(dot3(view_direction, microfacet));
                const float event_fresnel = can_refract ? fresnel : 1.0f;
                const float pdf = dielectric_reflection_pdf(hit.normal, view_direction, direction,
                                                             microfacet, hit.roughness, event_fresnel);
                const float geometry = smith_schlick(n_dot_v, hit.roughness) *
                                       smith_schlick(n_dot_l, hit.roughness);
                const float brdf = event_fresnel * ggx_distribution(n_dot_h, hit.roughness) * geometry /
                                   fmaxf(4.0f * n_dot_v * n_dot_l, 1.0e-8f);
                if (pdf <= 0.0f || n_dot_l <= 0.0f || v_dot_h <= 0.0f) break;
                throughput = mul(throughput, brdf * n_dot_l / pdf);
            } else {
                direction = transmitted;
                const float pdf = dielectric_transmission_pdf(hit.normal, view_direction, direction,
                                                               microfacet, hit.roughness, fresnel,
                                                               eta_incident, eta_transmitted);
                const float3 btdf = evaluate_dielectric_btdf(hit.normal, view_direction, direction,
                                                              microfacet, hit.albedo, hit.roughness,
                                                              fresnel, eta_incident, eta_transmitted);
                const float cosine = fabsf(dot3(hit.normal, direction));
                if (pdf <= 0.0f || cosine <= 0.0f) break;
                throughput = mul(throughput, mul(btdf, cosine / pdf));
            }
            previous_uses_mis = false;
        } else {
            const float choice = rng.next();
            const float coat_probability = clearcoat_probability(hit.clearcoat);
            const float base_specular_probability =
                coat_probability + (1.0f - coat_probability) * specular_probability(hit.metallic);
            if (choice < coat_probability) {
                const float3 microfacet = sample_ggx_normal(hit.normal, hit.clearcoat_roughness, rng);
                direction = normalize3(reflect3(ray.direction, microfacet));
            } else if (choice < base_specular_probability) {
                const float3 microfacet = sample_ggx_normal(hit.normal, hit.roughness, rng);
                direction = normalize3(reflect3(ray.direction, microfacet));
            } else {
                direction = cosine_direction(hit.normal, rng);
            }
            const float cosine = fmaxf(0.0f, dot3(hit.normal, direction));
            const float pdf = metallic_roughness_pdf(
                hit.normal, view_direction, direction, hit.metallic, hit.roughness,
                hit.clearcoat, hit.clearcoat_roughness);
            if (pdf <= 0.0f || cosine <= 0.0f) break;
            const float3 brdf = evaluate_metallic_roughness(
                hit.normal, view_direction, direction, hit.albedo, hit.metallic, hit.roughness,
                hit.clearcoat, hit.clearcoat_roughness, hit.sheen_color, hit.sheen_roughness);
            throughput = mul(throughput, mul(brdf, cosine / pdf));
            previous_bsdf_pdf = pdf;
            previous_uses_mis = true;
        }
        const float bias = dot3(direction, hit.normal) >= 0.0f ? 1.0e-4f : -1.0e-4f;
        ray = {add(hit.position, mul(hit.normal, bias)), direction};
        if (depth >= 3) {
            const float survival = fminf(0.95f, fmaxf(throughput.x, fmaxf(throughput.y, throughput.z)));
            if (rng.next() > survival) break;
            throughput = mul(throughput, 1.0f / survival);
        }
    }
    return result;
}

__global__ void render_tile(float3* tile, int tile_x, int tile_y, int tile_width, int tile_height,
                            int image_width, int image_height, int samples, std::uint32_t seed,
                            const Sphere* spheres, int sphere_count, const Triangle* triangles,
                            const BvhNode* nodes, int node_count, LightSet lights,
                            GpuEnvironment environment, GpuTextures textures, Camera camera) {
    const int local_x = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int local_y = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);
    if (local_x >= tile_width || local_y >= tile_height) return;
    const int x = tile_x + local_x;
    const int y = tile_y + local_y;
    const std::uint32_t pixel = static_cast<std::uint32_t>(y * image_width + x);
    float3 color = make_float3(0, 0, 0);
    for (int sample = 0; sample < samples; ++sample) {
        Rng rng(seed ^ (pixel * 747796405u + static_cast<std::uint32_t>(sample) * 2891336453u + 277803737u));
        const float u = (static_cast<float>(x) + rng.next()) / static_cast<float>(image_width);
        const float v = (static_cast<float>(y) + rng.next()) / static_cast<float>(image_height);
        const float aspect = static_cast<float>(image_width) / static_cast<float>(image_height);
        const float3 origin = camera.origin;
        const float3 target = camera.target;
        const float3 forward = normalize3(sub(target, origin));
        const float3 right = normalize3(cross3(forward, make_float3(0, 1, 0)));
        const float3 up = cross3(right, forward);
        const float viewport = tanf(camera.vertical_fov * kPi / 360.0f) * 2.0f;
        const float3 direction = normalize3(add(forward, add(mul(right, (u - 0.5f) * viewport * aspect),
                                                               mul(up, (0.5f - v) * viewport))));
        color = add(color, radiance({origin, direction}, spheres, sphere_count,
                                    triangles, nodes, node_count, lights, environment, textures, rng));
    }
    tile[local_y * tile_width + local_x] = mul(color, 1.0f / static_cast<float>(samples));
}

__global__ void probe_bvh(const Triangle* triangles, const BvhNode* nodes, int node_count, int* passed) {
    Hit hit{};
    hit.distance = kInfinity;
    hit.found = false;
    intersect_triangles({make_float3(0, 1, 0), make_float3(0, -1, 0)},
                        triangles, nodes, node_count, hit);
    *passed = hit.found && fabsf(hit.distance - 1.0f) < 1.0e-5f;
}

__global__ void probe_dielectric(int* passed) {
    float3 straight{};
    float3 total_internal{};
    const bool normal_refraction = refract3(make_float3(0, -1, 0), make_float3(0, 1, 0),
                                             1.0f / 1.5f, straight);
    const bool total_internal_reflection = !refract3(normalize3(make_float3(0.8660254f, -0.5f, 0)),
                                                      make_float3(0, 1, 0), 1.5f, total_internal);
    const float3 normal = make_float3(0, 1, 0);
    const float3 view = make_float3(0, 1, 0);
    const float3 transmitted = make_float3(0, -1, 0);
    const float fresnel = fresnel_dielectric(1.0f, 1.0f, 1.5f);
    const float pdf = dielectric_transmission_pdf(normal, view, transmitted, normal, 0.3f,
                                                  fresnel, 1.0f, 1.5f);
    const float3 btdf = evaluate_dielectric_btdf(normal, view, transmitted, normal,
                                                 make_float3(0.9f, 0.95f, 1.0f), 0.3f,
                                                 fresnel, 1.0f, 1.5f);
    const float3 weight = pdf > 0.0f ? mul(btdf, 1.0f / pdf) : make_float3(0, 0, 0);
    *passed = fabsf(fresnel_dielectric(1.0f, 1.0f, 1.5f) - 0.04f) < 1.0e-5f &&
              normal_refraction && fabsf(straight.y + 1.0f) < 1.0e-5f && total_internal_reflection &&
              pdf > 0.0f && isfinite(weight.x) && weight.x > 0.0f;
}

__global__ void probe_mis(int* passed) {
    const AreaLight light{make_float3(0, 2, 0), make_float3(1, 0, 0),
                          make_float3(0, 0, 1), make_float3(1, 1, 1)};
    const Ray ray{make_float3(0, 0, 0), make_float3(0, 1, 0)};
    float distance = 0.0f;
    const bool intersects = intersect_light(light, ray, distance);
    *passed = fabsf(power_heuristic(0.25f, 0.25f) - 0.5f) < 1.0e-6f && intersects &&
              fabsf(distance - 2.0f) < 1.0e-6f &&
              fabsf(area_light_pdf(light, ray, distance) - 1.0f) < 1.0e-6f;
}

__global__ void probe_environment(GpuEnvironment environment, int* passed) {
    const float3 color = sample_environment(environment, make_float3(1, 0, 0));
    Rng rng(42u);
    float3 direction{};
    float3 sampled{};
    float pdf = 0.0f;
    const bool sampled_direction = sample_environment_direction(environment, rng, direction, sampled, pdf);
    *passed = fabsf(color.x - 2.0f) < 1.0e-6f && fabsf(color.y - 1.0f) < 1.0e-6f &&
              fabsf(color.z - 0.5f) < 1.0e-6f && sampled_direction &&
              fabsf(dot3(direction, direction) - 1.0f) < 1.0e-5f &&
              fabsf(pdf - 1.0f / (4.0f * kPi)) < 1.0e-5f;
}

__global__ void probe_ggx(int* passed) {
    const float3 normal = make_float3(0, 1, 0);
    const float3 view = make_float3(0, 1, 0);
    Rng rng(91u);
    const float3 half_vector = sample_ggx_normal(normal, 0.35f, rng);
    const float3 light = normalize3(reflect3(mul(view, -1.0f), half_vector));
    const float pdf = ggx_reflection_pdf(normal, view, light, 0.35f);
    const float3 brdf = evaluate_ggx_metal(normal, view, light, make_float3(0.8f, 0.6f, 0.2f), 0.35f);
    const float cosine = fmaxf(0.0f, dot3(normal, light));
    const float3 weight = pdf > 0.0f ? mul(brdf, cosine / pdf) : make_float3(0, 0, 0);
    *passed = ggx_distribution(1.0f, 0.1f) > ggx_distribution(1.0f, 0.8f) &&
              pdf > 0.0f && isfinite(weight.x) && isfinite(weight.y) && isfinite(weight.z) &&
              weight.x > 0.0f;
}

float component(float3 value, int axis) { return axis == 0 ? value.x : axis == 1 ? value.y : value.z; }
float3 minimum3(float3 a, float3 b) {
    return make_float3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}
float3 maximum3(float3 a, float3 b) {
    return make_float3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}
float3 centroid(const Triangle& triangle) {
    return mul(add(add(triangle.first, triangle.second), triangle.third), 1.0f / 3.0f);
}

std::vector<BvhNode> build_bvh(std::vector<Triangle>& triangles) {
    if (triangles.empty()) return {};
    std::vector<BvhNode> nodes;
    nodes.reserve(triangles.size() * 2);
    const auto build = [&](auto&& self, int begin, int end) -> int {
        float3 minimum = make_float3(kInfinity, kInfinity, kInfinity);
        float3 maximum = make_float3(-kInfinity, -kInfinity, -kInfinity);
        float3 centroid_minimum = minimum;
        float3 centroid_maximum = maximum;
        for (int index = begin; index < end; ++index) {
            const Triangle& triangle = triangles[index];
            minimum = minimum3(minimum, minimum3(triangle.first, minimum3(triangle.second, triangle.third)));
            maximum = maximum3(maximum, maximum3(triangle.first, maximum3(triangle.second, triangle.third)));
            const float3 center = centroid(triangle);
            centroid_minimum = minimum3(centroid_minimum, center);
            centroid_maximum = maximum3(centroid_maximum, center);
        }
        const int node_index = static_cast<int>(nodes.size());
        nodes.push_back({minimum, maximum, -1, -1, begin, end - begin});
        if (end - begin <= 1) return node_index;
        const float3 extent = sub(centroid_maximum, centroid_minimum);
        const int axis = extent.y > extent.x && extent.y >= extent.z ? 1 : extent.z > extent.x ? 2 : 0;
        const int middle = begin + (end - begin) / 2;
        std::nth_element(triangles.begin() + begin, triangles.begin() + middle, triangles.begin() + end,
                         [axis](const Triangle& lhs, const Triangle& rhs) {
                             return component(centroid(lhs), axis) < component(centroid(rhs), axis);
                         });
        const int left = self(self, begin, middle);
        const int right = self(self, middle, end);
        nodes[node_index].left = left;
        nodes[node_index].right = right;
        nodes[node_index].count = 0;
        return node_index;
    };
    build(build, 0, static_cast<int>(triangles.size()));
    return nodes;
}

void validate_bvh_on_gpu() {
    std::vector<Triangle> triangles{
        {make_float3(-1, 0, -1), make_float3(1, 0, -1), make_float3(0, 0, 1),
         make_float3(1, 1, 1), 0.0f, 0.5f, 0.0f, 1.5f},
    };
    const std::vector<BvhNode> nodes = build_bvh(triangles);
    Triangle* device_triangles = nullptr;
    BvhNode* device_nodes = nullptr;
    int* device_passed = nullptr;
    try {
        check(cudaMalloc(&device_triangles, sizeof(Triangle)), "cudaMalloc BVH probe triangle");
        check(cudaMalloc(&device_nodes, nodes.size() * sizeof(BvhNode)), "cudaMalloc BVH probe nodes");
        check(cudaMalloc(&device_passed, sizeof(int)), "cudaMalloc BVH probe result");
        check(cudaMemcpy(device_triangles, triangles.data(), sizeof(Triangle), cudaMemcpyHostToDevice),
              "copy BVH probe triangle");
        check(cudaMemcpy(device_nodes, nodes.data(), nodes.size() * sizeof(BvhNode), cudaMemcpyHostToDevice),
              "copy BVH probe nodes");
        probe_bvh<<<1, 1>>>(device_triangles, device_nodes, static_cast<int>(nodes.size()), device_passed);
        check(cudaGetLastError(), "BVH probe launch");
        int passed = 0;
        check(cudaMemcpy(&passed, device_passed, sizeof(int), cudaMemcpyDeviceToHost), "copy BVH probe result");
        check(cudaFree(device_passed), "cudaFree BVH probe result");
        device_passed = nullptr;
        check(cudaFree(device_nodes), "cudaFree BVH probe nodes");
        device_nodes = nullptr;
        check(cudaFree(device_triangles), "cudaFree BVH probe triangle");
        device_triangles = nullptr;
        if (!passed) throw std::runtime_error("GPU triangle BVH probe missed its known intersection");
    } catch (...) {
        if (device_passed) cudaFree(device_passed);
        if (device_nodes) cudaFree(device_nodes);
        if (device_triangles) cudaFree(device_triangles);
        throw;
    }
}

void validate_dielectric_on_gpu() {
    int* device_passed = nullptr;
    try {
        check(cudaMalloc(&device_passed, sizeof(int)), "cudaMalloc dielectric probe result");
        probe_dielectric<<<1, 1>>>(device_passed);
        check(cudaGetLastError(), "dielectric probe launch");
        int passed = 0;
        check(cudaMemcpy(&passed, device_passed, sizeof(int), cudaMemcpyDeviceToHost),
              "copy dielectric probe result");
        check(cudaFree(device_passed), "cudaFree dielectric probe result");
        device_passed = nullptr;
        if (!passed) throw std::runtime_error("GPU dielectric Fresnel, refraction, or TIR probe failed");
    } catch (...) {
        if (device_passed) cudaFree(device_passed);
        throw;
    }
}

void validate_mis_on_gpu() {
    int* device_passed = nullptr;
    try {
        check(cudaMalloc(&device_passed, sizeof(int)), "cudaMalloc MIS probe result");
        probe_mis<<<1, 1>>>(device_passed);
        check(cudaGetLastError(), "MIS probe launch");
        int passed = 0;
        check(cudaMemcpy(&passed, device_passed, sizeof(int), cudaMemcpyDeviceToHost), "copy MIS probe result");
        check(cudaFree(device_passed), "cudaFree MIS probe result");
        device_passed = nullptr;
        if (!passed) throw std::runtime_error("GPU area-light PDF or MIS power heuristic probe failed");
    } catch (...) {
        if (device_passed) cudaFree(device_passed);
        throw;
    }
}

void validate_environment_on_gpu() {
    const std::vector<float3> pixels(4, make_float3(2.0f, 1.0f, 0.5f));
    const std::vector<float> pmf(4, 0.25f);
    const std::vector<float> cdf{0.25f, 0.5f, 0.75f, 1.0f};
    float3* device_pixels = nullptr;
    float* device_pmf = nullptr;
    float* device_cdf = nullptr;
    int* device_passed = nullptr;
    try {
        check(cudaMalloc(&device_pixels, pixels.size() * sizeof(float3)), "cudaMalloc environment probe pixels");
        check(cudaMalloc(&device_pmf, pmf.size() * sizeof(float)), "cudaMalloc environment probe PMF");
        check(cudaMalloc(&device_cdf, cdf.size() * sizeof(float)), "cudaMalloc environment probe CDF");
        check(cudaMalloc(&device_passed, sizeof(int)), "cudaMalloc environment probe result");
        check(cudaMemcpy(device_pixels, pixels.data(), pixels.size() * sizeof(float3), cudaMemcpyHostToDevice),
              "copy environment probe pixels");
        check(cudaMemcpy(device_pmf, pmf.data(), pmf.size() * sizeof(float), cudaMemcpyHostToDevice),
              "copy environment probe PMF");
        check(cudaMemcpy(device_cdf, cdf.data(), cdf.size() * sizeof(float), cudaMemcpyHostToDevice),
              "copy environment probe CDF");
        probe_environment<<<1, 1>>>({device_pixels, device_pmf, device_cdf, 2, 2}, device_passed);
        check(cudaGetLastError(), "environment probe launch");
        int passed = 0;
        check(cudaMemcpy(&passed, device_passed, sizeof(int), cudaMemcpyDeviceToHost),
              "copy environment probe result");
        check(cudaFree(device_passed), "cudaFree environment probe result");
        device_passed = nullptr;
        check(cudaFree(device_pixels), "cudaFree environment probe pixels");
        device_pixels = nullptr;
        check(cudaFree(device_cdf), "cudaFree environment probe CDF");
        device_cdf = nullptr;
        check(cudaFree(device_pmf), "cudaFree environment probe PMF");
        device_pmf = nullptr;
        if (!passed) throw std::runtime_error("GPU HDR environment filtering clipped or changed linear values");
    } catch (...) {
        if (device_passed) cudaFree(device_passed);
        if (device_cdf) cudaFree(device_cdf);
        if (device_pmf) cudaFree(device_pmf);
        if (device_pixels) cudaFree(device_pixels);
        throw;
    }
}

void validate_ggx_on_gpu() {
    int* device_passed = nullptr;
    try {
        check(cudaMalloc(&device_passed, sizeof(int)), "cudaMalloc GGX probe result");
        probe_ggx<<<1, 1>>>(device_passed);
        check(cudaGetLastError(), "GGX probe launch");
        int passed = 0;
        check(cudaMemcpy(&passed, device_passed, sizeof(int), cudaMemcpyDeviceToHost), "copy GGX probe result");
        check(cudaFree(device_passed), "cudaFree GGX probe result");
        device_passed = nullptr;
        if (!passed) throw std::runtime_error("GPU GGX BRDF, sampling, or reflection PDF probe failed");
    } catch (...) {
        if (device_passed) cudaFree(device_passed);
        throw;
    }
}

ImportedAssets load_gltf_triangles(const std::filesystem::path& path) {
    const cg::assets::GltfAsset asset = cg::assets::GltfAsset::load(path);
    ImportedAssets result;
    result.triangles.reserve(asset.triangles().size());
    std::vector<std::shared_ptr<const cg::assets::GltfTexture>> texture_sources;
    std::vector<bool> texture_srgb;
    const auto convert = [](const cg::Vec3& value) {
        return make_float3(static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z));
    };
    const auto register_texture = [&](const std::shared_ptr<const cg::assets::GltfTexture>& texture,
                                      bool srgb) {
        if (!texture) return -1;
        for (std::size_t index = 0; index < texture_sources.size(); ++index)
            if (texture_sources[index] == texture && texture_srgb[index] == srgb) return static_cast<int>(index);
        const int index = static_cast<int>(texture_sources.size());
        texture_sources.push_back(texture);
        texture_srgb.push_back(srgb);
        result.textures.push_back({static_cast<int>(result.texture_pixels.size()),
                                   static_cast<int>(texture->width), static_cast<int>(texture->height)});
        for (const cg::Color& pixel : texture->pixels) {
            const cg::Color value = srgb ? cg::Color{std::pow(pixel.x, 2.2), std::pow(pixel.y, 2.2),
                                                     std::pow(pixel.z, 2.2)} : pixel;
            result.texture_pixels.push_back(convert(value));
        }
        return index;
    };
    for (const cg::assets::GltfTriangle& triangle : asset.triangles()) {
        const int texture_index = register_texture(triangle.base_color_texture, true);
        const int normal_texture_index = register_texture(triangle.normal_texture, false);
        const int metallic_roughness_texture_index =
            register_texture(triangle.metallic_roughness_texture, false);
        const int emissive_texture_index = register_texture(triangle.emissive_texture, true);
        const int clearcoat_texture_index = register_texture(triangle.clearcoat_texture, false);
        const float3 first = convert(triangle.first), second = convert(triangle.second), third = convert(triangle.third);
        const float2 uv0 = make_float2(static_cast<float>(triangle.first_uv.x), static_cast<float>(triangle.first_uv.y));
        const float2 uv1 = make_float2(static_cast<float>(triangle.second_uv.x), static_cast<float>(triangle.second_uv.y));
        const float2 uv2 = make_float2(static_cast<float>(triangle.third_uv.x), static_cast<float>(triangle.third_uv.y));
        const float determinant = (uv1.x - uv0.x) * (uv2.y - uv0.y) - (uv1.y - uv0.y) * (uv2.x - uv0.x);
        float3 tangent = make_float3(1, 0, 0);
        float handedness = 1.0f;
        if (fabsf(determinant) > 1.0e-8f) {
            const float inverse = 1.0f / determinant;
            const float3 edge1 = sub(second, first), edge2 = sub(third, first);
            tangent = normalize3(mul(sub(mul(edge1, uv2.y - uv0.y), mul(edge2, uv1.y - uv0.y)), inverse));
            const float3 bitangent = normalize3(mul(sub(mul(edge2, uv1.x - uv0.x), mul(edge1, uv2.x - uv0.x)), inverse));
            const float3 normal = triangle.has_normals ? convert(triangle.first_normal)
                                                        : normalize3(cross3(edge1, edge2));
            handedness = dot3(cross3(normal, tangent), bitangent) < 0.0f ? -1.0f : 1.0f;
        }
        result.triangles.push_back({first, second, third,
                          convert(triangle.material.albedo), static_cast<float>(triangle.material.metallic),
                          static_cast<float>(triangle.material.roughness),
                          static_cast<float>(triangle.material.transmission),
                          static_cast<float>(triangle.material.index_of_refraction),
                          convert(triangle.first_normal), convert(triangle.second_normal),
                          convert(triangle.third_normal), triangle.has_normals,
                          uv0, uv1, uv2, texture_index, tangent, handedness, normal_texture_index,
                          static_cast<float>(triangle.normal_scale), metallic_roughness_texture_index,
                          convert(triangle.emissive_factor), emissive_texture_index,
                          triangle.material_index, static_cast<float>(triangle.clearcoat_factor),
                          fmaxf(0.04f, static_cast<float>(triangle.clearcoat_roughness)),
                          clearcoat_texture_index, convert(triangle.sheen_color_factor),
                          static_cast<float>(triangle.sheen_roughness)});
    }
    return result;
}

void append_imported(ImportedAssets& destination, ImportedAssets source) {
    const int texture_base = static_cast<int>(destination.textures.size());
    const int pixel_base = static_cast<int>(destination.texture_pixels.size());
    for (TextureDescriptor& texture : source.textures) {
        texture.offset += pixel_base;
        destination.textures.push_back(texture);
    }
    destination.texture_pixels.insert(destination.texture_pixels.end(),
                                      source.texture_pixels.begin(), source.texture_pixels.end());
    for (Triangle& triangle : source.triangles) {
        if (triangle.texture_index >= 0) triangle.texture_index += texture_base;
        if (triangle.normal_texture_index >= 0) triangle.normal_texture_index += texture_base;
        if (triangle.metallic_roughness_texture_index >= 0)
            triangle.metallic_roughness_texture_index += texture_base;
        if (triangle.emissive_texture_index >= 0) triangle.emissive_texture_index += texture_base;
        if (triangle.clearcoat_texture_index >= 0) triangle.clearcoat_texture_index += texture_base;
        destination.triangles.push_back(triangle);
    }
}

void frame_imported_triangles(std::vector<Triangle>& triangles, float target_extent = 3.2f,
                              float target_x = 0.0f, float target_z = -4.5f,
                              int excluded_bounds_material = -1) {
    if (triangles.empty()) return;
    const auto contributes_to_bounds = [&](const Triangle& triangle) {
        return triangle.material_index != excluded_bounds_material;
    };
    const auto first = std::find_if(triangles.begin(), triangles.end(), contributes_to_bounds);
    if (first == triangles.end()) throw std::runtime_error("no triangles remain for subject framing");
    float3 minimum = first->first;
    float3 maximum = minimum;
    const auto include = [&](const float3& point) {
        minimum.x = std::min(minimum.x, point.x);
        minimum.y = std::min(minimum.y, point.y);
        minimum.z = std::min(minimum.z, point.z);
        maximum.x = std::max(maximum.x, point.x);
        maximum.y = std::max(maximum.y, point.y);
        maximum.z = std::max(maximum.z, point.z);
    };
    for (const Triangle& triangle : triangles) {
        if (!contributes_to_bounds(triangle)) continue;
        include(triangle.first);
        include(triangle.second);
        include(triangle.third);
    }
    const float largest_extent = std::max(maximum.x - minimum.x,
                                          std::max(maximum.y - minimum.y, maximum.z - minimum.z));
    if (!(largest_extent > 1.0e-6f) || !std::isfinite(largest_extent))
        throw std::runtime_error("glTF mesh has a degenerate bounding box");
    const float scale = target_extent / largest_extent;
    const float center_x = (minimum.x + maximum.x) * 0.5f;
    const float center_z = (minimum.z + maximum.z) * 0.5f;
    const auto frame = [&](float3& point) {
        point = make_float3((point.x - center_x) * scale + target_x,
                            (point.y - minimum.y) * scale - 1.0f,
                            (point.z - center_z) * scale + target_z);
    };
    for (Triangle& triangle : triangles) {
        frame(triangle.first);
        frame(triangle.second);
        frame(triangle.third);
    }
}

struct EnvironmentData {
    std::vector<float3> pixels;
    std::vector<float> pmf;
    std::vector<float> cdf;
};

EnvironmentData build_environment_data(const cg::environment::EnvironmentMap* environment) {
    EnvironmentData result;
    if (!environment) return result;
    const std::size_t count = environment->pixels().size();
    result.pixels.reserve(count);
    result.pmf.resize(count);
    result.cdf.resize(count);
    double total = 0.0;
    for (std::size_t y = 0; y < environment->height(); ++y) {
        const double theta = kPi * (static_cast<double>(y) + 0.5) / static_cast<double>(environment->height());
        const double sine = std::sin(theta);
        for (std::size_t x = 0; x < environment->width(); ++x) {
            const std::size_t index = y * environment->width() + x;
            const cg::Color color = environment->pixels()[index] * environment->intensity();
            result.pixels.push_back(make_float3(static_cast<float>(color.x), static_cast<float>(color.y),
                                                static_cast<float>(color.z)));
            const double luminance = 0.2126 * color.x + 0.7152 * color.y + 0.0722 * color.z;
            result.pmf[index] = static_cast<float>(std::max(0.0, luminance) * sine);
            total += result.pmf[index];
        }
    }
    if (total <= 0.0) {
        std::fill(result.pmf.begin(), result.pmf.end(), 1.0f / static_cast<float>(count));
    } else {
        for (float& probability : result.pmf) probability = static_cast<float>(probability / total);
    }
    float cumulative = 0.0f;
    for (std::size_t index = 0; index < count; ++index) {
        cumulative += result.pmf[index];
        result.cdf[index] = cumulative;
    }
    result.cdf.back() = 1.0f;
    return result;
}

std::vector<float3> render(int width, int height, int samples, std::uint32_t seed,
                           const ImportedAssets& imported = {}, bool show_demo_spheres = true,
                           const cg::environment::EnvironmentMap* environment_map = nullptr,
                           Camera camera = {make_float3(0.0f, 0.65f, 2.8f),
                                            make_float3(0.0f, -0.05f, -4.0f), 48.0f},
                           bool studio_lighting = false) {
    LightSet lights{{
        {make_float3(-0.8f, 4.5f, -3.5f), make_float3(1.5f, 0, 0),
         make_float3(0, 0, 1.0f), make_float3(10.0f, 8.0f, 5.5f)},
        {}, {}
    }, 1};
    if (studio_lighting) {
        lights = {{
            {make_float3(-3.2f, 3.8f, -1.8f), make_float3(1.2f, 0, 0.35f),
             make_float3(0.15f, 0, 1.0f), make_float3(15.0f, 9.0f, 5.0f)},
            {make_float3(3.5f, 2.1f, -2.5f), make_float3(0.8f, 0, -0.25f),
             make_float3(-0.1f, 0, 0.8f), make_float3(3.0f, 5.0f, 9.0f)},
            {make_float3(2.5f, 5.8f, -7.5f), make_float3(1.0f, 0, 0),
             make_float3(0, 0.65f, 0.35f), make_float3(8.0f, 10.0f, 14.0f)}
        }, 3};
    }
    std::vector<Sphere> spheres;
    if (show_demo_spheres) {
        spheres = {
            {make_float3(-1.3f, -0.05f, -4.2f), 0.95f, make_float3(0.85f, 0.12f, 0.06f), 0.0f, 0.5f, 0.0f, 1.5f},
            {make_float3(1.25f, -0.15f, -4.1f), 0.85f, make_float3(0.88f, 0.9f, 0.95f), 1.0f, 0.28f, 0.0f, 1.5f},
            {make_float3(0.0f, 0.55f, -5.2f), 0.65f, make_float3(0.72f, 0.88f, 1.0f), 0.0f, 0.12f, 0.94f, 1.5f},
        };
    }
    std::vector<Triangle> triangles{
        {make_float3(-7, -1, 2), make_float3(7, -1, 2), make_float3(7, -1, -12),
         make_float3(0.65f, 0.68f, 0.72f), 0.0f, 0.8f, 0.0f, 1.5f},
        {make_float3(-7, -1, 2), make_float3(7, -1, -12), make_float3(-7, -1, -12),
         make_float3(0.65f, 0.68f, 0.72f), 0.0f, 0.8f, 0.0f, 1.5f},
        {make_float3(-7, -1, -9), make_float3(7, -1, -9), make_float3(7, 6, -9),
         make_float3(0.3f, 0.38f, 0.5f), 0.0f, 0.7f, 0.0f, 1.5f},
        {make_float3(-7, -1, -9), make_float3(7, 6, -9), make_float3(-7, 6, -9),
         make_float3(0.3f, 0.38f, 0.5f), 0.0f, 0.7f, 0.0f, 1.5f},
    };
    triangles.insert(triangles.end(), imported.triangles.begin(), imported.triangles.end());
    const std::vector<BvhNode> nodes = build_bvh(triangles);
    const EnvironmentData environment_data = build_environment_data(environment_map);
    Sphere* device_spheres = nullptr;
    Triangle* device_triangles = nullptr;
    BvhNode* device_nodes = nullptr;
    float3* device_environment_pixels = nullptr;
    float* device_environment_pmf = nullptr;
    float* device_environment_cdf = nullptr;
    float3* device_texture_pixels = nullptr;
    TextureDescriptor* device_texture_descriptors = nullptr;
    float3* device_tile = nullptr;
    try {
        if (!spheres.empty()) check(cudaMalloc(&device_spheres, spheres.size() * sizeof(Sphere)), "cudaMalloc spheres");
        check(cudaMalloc(&device_triangles, triangles.size() * sizeof(Triangle)), "cudaMalloc triangles");
        check(cudaMalloc(&device_nodes, nodes.size() * sizeof(BvhNode)), "cudaMalloc BVH nodes");
        if (!environment_data.pixels.empty()) {
            check(cudaMalloc(&device_environment_pixels, environment_data.pixels.size() * sizeof(float3)),
                  "cudaMalloc HDR environment");
            check(cudaMalloc(&device_environment_pmf, environment_data.pmf.size() * sizeof(float)),
                  "cudaMalloc HDR PMF");
            check(cudaMalloc(&device_environment_cdf, environment_data.cdf.size() * sizeof(float)),
                  "cudaMalloc HDR CDF");
        }
        if (!imported.texture_pixels.empty()) {
            check(cudaMalloc(&device_texture_pixels, imported.texture_pixels.size() * sizeof(float3)),
                  "cudaMalloc texture pixels");
            check(cudaMalloc(&device_texture_descriptors, imported.textures.size() * sizeof(TextureDescriptor)),
                  "cudaMalloc texture descriptors");
            check(cudaMemcpy(device_texture_pixels, imported.texture_pixels.data(),
                             imported.texture_pixels.size() * sizeof(float3), cudaMemcpyHostToDevice),
                  "copy texture pixels");
            check(cudaMemcpy(device_texture_descriptors, imported.textures.data(),
                             imported.textures.size() * sizeof(TextureDescriptor), cudaMemcpyHostToDevice),
                  "copy texture descriptors");
        }
        check(cudaMalloc(&device_tile, kTileWidth * kTileHeight * sizeof(float3)), "cudaMalloc tile");
        if (!spheres.empty())
            check(cudaMemcpy(device_spheres, spheres.data(), spheres.size() * sizeof(Sphere), cudaMemcpyHostToDevice),
                  "copy spheres");
        check(cudaMemcpy(device_triangles, triangles.data(), triangles.size() * sizeof(Triangle), cudaMemcpyHostToDevice),
              "copy triangles");
        check(cudaMemcpy(device_nodes, nodes.data(), nodes.size() * sizeof(BvhNode), cudaMemcpyHostToDevice),
              "copy BVH nodes");
        if (!environment_data.pixels.empty()) {
            check(cudaMemcpy(device_environment_pixels, environment_data.pixels.data(),
                             environment_data.pixels.size() * sizeof(float3), cudaMemcpyHostToDevice),
                  "copy HDR environment");
            check(cudaMemcpy(device_environment_pmf, environment_data.pmf.data(),
                             environment_data.pmf.size() * sizeof(float), cudaMemcpyHostToDevice),
                  "copy HDR PMF");
            check(cudaMemcpy(device_environment_cdf, environment_data.cdf.data(),
                             environment_data.cdf.size() * sizeof(float), cudaMemcpyHostToDevice),
                  "copy HDR CDF");
        }
        const GpuEnvironment environment{device_environment_pixels, device_environment_pmf,
                                         device_environment_cdf,
                                         environment_map ? static_cast<int>(environment_map->width()) : 0,
                                         environment_map ? static_cast<int>(environment_map->height()) : 0};
        const GpuTextures textures{device_texture_pixels, device_texture_descriptors,
                                   static_cast<int>(imported.textures.size())};
        std::vector<float3> image(static_cast<std::size_t>(width) * height);
        std::vector<float3> tile(kTileWidth * kTileHeight);
        for (int tile_y = 0; tile_y < height; tile_y += kTileHeight) {
            for (int tile_x = 0; tile_x < width; tile_x += kTileWidth) {
                const int tile_width = std::min(kTileWidth, width - tile_x);
                const int tile_height = std::min(kTileHeight, height - tile_y);
                const dim3 threads(8, 8);
                const dim3 blocks((tile_width + 7) / 8, (tile_height + 7) / 8);
                render_tile<<<blocks, threads>>>(device_tile, tile_x, tile_y, tile_width, tile_height,
                                                  width, height, samples, seed, device_spheres,
                                                  static_cast<int>(spheres.size()), device_triangles,
                                                  device_nodes, static_cast<int>(nodes.size()), lights, environment,
                                                  textures, camera);
                check(cudaGetLastError(), "render tile launch");
                check(cudaMemcpy(tile.data(), device_tile, tile_width * tile_height * sizeof(float3),
                                 cudaMemcpyDeviceToHost), "copy rendered tile");
                for (int y = 0; y < tile_height; ++y)
                    for (int x = 0; x < tile_width; ++x)
                        image[static_cast<std::size_t>(tile_y + y) * width + tile_x + x] =
                            tile[static_cast<std::size_t>(y) * tile_width + x];
            }
        }
        check(cudaFree(device_tile), "cudaFree tile");
        device_tile = nullptr;
        if (device_environment_cdf) check(cudaFree(device_environment_cdf), "cudaFree HDR CDF");
        device_environment_cdf = nullptr;
        if (device_environment_pmf) check(cudaFree(device_environment_pmf), "cudaFree HDR PMF");
        device_environment_pmf = nullptr;
        if (device_environment_pixels) check(cudaFree(device_environment_pixels), "cudaFree HDR environment");
        device_environment_pixels = nullptr;
        if (device_texture_descriptors) check(cudaFree(device_texture_descriptors), "cudaFree texture descriptors");
        device_texture_descriptors = nullptr;
        if (device_texture_pixels) check(cudaFree(device_texture_pixels), "cudaFree texture pixels");
        device_texture_pixels = nullptr;
        check(cudaFree(device_nodes), "cudaFree BVH nodes");
        device_nodes = nullptr;
        check(cudaFree(device_triangles), "cudaFree triangles");
        device_triangles = nullptr;
        if (device_spheres) check(cudaFree(device_spheres), "cudaFree spheres");
        device_spheres = nullptr;
        return image;
    } catch (...) {
        if (device_tile) cudaFree(device_tile);
        if (device_environment_cdf) cudaFree(device_environment_cdf);
        if (device_environment_pmf) cudaFree(device_environment_pmf);
        if (device_environment_pixels) cudaFree(device_environment_pixels);
        if (device_texture_descriptors) cudaFree(device_texture_descriptors);
        if (device_texture_pixels) cudaFree(device_texture_pixels);
        if (device_nodes) cudaFree(device_nodes);
        if (device_triangles) cudaFree(device_triangles);
        if (device_spheres) cudaFree(device_spheres);
        throw;
    }
}

unsigned char channel(float value) {
    value = std::sqrt(std::min(1.0f, std::max(0.0f, value)));
    return static_cast<unsigned char>(value * 255.0f + 0.5f);
}

template <typename Integer>
void write_little_endian(std::ostream& output, Integer value) {
    using Unsigned = std::make_unsigned_t<Integer>;
    const Unsigned bits = static_cast<Unsigned>(value);
    for (std::size_t index = 0; index < sizeof(Integer); ++index)
        output.put(static_cast<char>((bits >> (index * 8u)) & 0xffu));
}

void write_ppm(const std::string& path, int width, int height, const std::vector<float3>& pixels) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("failed to open output: " + path);
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (float3 color : pixels) {
        const unsigned char bytes[]{channel(color.x), channel(color.y), channel(color.z)};
        output.write(reinterpret_cast<const char*>(bytes), 3);
    }
}

void write_bmp(const std::string& path, int width, int height, const std::vector<float3>& pixels) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("failed to open output: " + path);
    const std::uint32_t row_size = (static_cast<std::uint32_t>(width) * 3u + 3u) & ~3u;
    const std::uint32_t pixel_bytes = row_size * static_cast<std::uint32_t>(height);
    constexpr std::uint32_t data_offset = 54;
    output.write("BM", 2);
    write_little_endian(output, data_offset + pixel_bytes);
    write_little_endian(output, std::uint32_t{0});
    write_little_endian(output, data_offset);
    write_little_endian(output, std::uint32_t{40});
    write_little_endian(output, static_cast<std::int32_t>(width));
    write_little_endian(output, -static_cast<std::int32_t>(height));
    write_little_endian(output, std::uint16_t{1});
    write_little_endian(output, std::uint16_t{24});
    write_little_endian(output, std::uint32_t{0});
    write_little_endian(output, pixel_bytes);
    write_little_endian(output, std::int32_t{2835});
    write_little_endian(output, std::int32_t{2835});
    write_little_endian(output, std::uint32_t{0});
    write_little_endian(output, std::uint32_t{0});
    const unsigned char padding[3]{};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float3 color = pixels[static_cast<std::size_t>(y) * width + x];
            const unsigned char bytes[]{channel(color.z), channel(color.y), channel(color.x)};
            output.write(reinterpret_cast<const char*>(bytes), 3);
        }
        output.write(reinterpret_cast<const char*>(padding), row_size - static_cast<std::uint32_t>(width) * 3u);
    }
}

void write_image(const std::string& path, int width, int height, const std::vector<float3>& pixels) {
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".bmp") write_bmp(path, width, height, pixels);
    else write_ppm(path, width, height, pixels);
}

void self_test() {
    validate_bvh_on_gpu();
    validate_dielectric_on_gpu();
    validate_mis_on_gpu();
    validate_environment_on_gpu();
    validate_ggx_on_gpu();
    const std::filesystem::path gltf_path = "cuda_gltf_test.gltf";
    const std::filesystem::path bin_path = "cuda_gltf_test.bin";
    {
        std::ofstream binary(bin_path, std::ios::binary);
        const float positions[]{-0.7f, -0.8f, -2.8f, 0.7f, -0.8f, -2.8f, 0.0f, 0.7f, -2.8f};
        binary.write(reinterpret_cast<const char*>(positions), sizeof(positions));
    }
    {
        std::ofstream gltf(gltf_path);
        gltf << R"({"asset":{"version":"2.0"},"buffers":[{"uri":"cuda_gltf_test.bin","byteLength":36}],
"bufferViews":[{"buffer":0,"byteLength":36}],
"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"}],
"materials":[{"pbrMetallicRoughness":{"baseColorFactor":[0.1,0.8,0.2,1],"metallicFactor":0.7,"roughnessFactor":0.35}}],
"meshes":[{"primitives":[{"attributes":{"POSITION":0},"material":0}]}],
"nodes":[{"mesh":0}],"scenes":[{"nodes":[0]}],"scene":0})";
    }
    ImportedAssets imported;
    try {
        imported = load_gltf_triangles(gltf_path);
        std::filesystem::remove(gltf_path);
        std::filesystem::remove(bin_path);
    } catch (...) {
        std::filesystem::remove(gltf_path);
        std::filesystem::remove(bin_path);
        throw;
    }
    if (imported.triangles.size() != 1 || fabsf(imported.triangles[0].metallic - 0.7f) > 1.0e-6f ||
        fabsf(imported.triangles[0].roughness - 0.35f) > 1.0e-6f)
        throw std::runtime_error("glTF triangles or PBR factors did not reach the CUDA upload format");
    frame_imported_triangles(imported.triangles);
    const float framed_minimum_y = std::min(imported.triangles[0].first.y,
                                             std::min(imported.triangles[0].second.y, imported.triangles[0].third.y));
    const float framed_maximum_y = std::max(imported.triangles[0].first.y,
                                             std::max(imported.triangles[0].second.y, imported.triangles[0].third.y));
    if (fabsf(framed_minimum_y + 1.0f) > 1.0e-5f ||
        fabsf(framed_maximum_y - framed_minimum_y - 3.2f) > 1.0e-5f)
        throw std::runtime_error("glTF automatic framing did not scale and place the mesh on the floor");
    const cg::environment::EnvironmentMap importance_environment(
        2, 2, {{40.0, 40.0, 40.0}, {0.1, 0.1, 0.1}, {0.1, 0.1, 0.1}, {0.1, 0.1, 0.1}});
    const EnvironmentData importance_data = build_environment_data(&importance_environment);
    if (importance_data.pmf[0] < 0.99f || importance_data.cdf.back() != 1.0f)
        throw std::runtime_error("HDR luminance distribution did not concentrate probability on the bright texel");
    const cg::environment::EnvironmentMap environment(
        2, 2, std::vector<cg::Color>(4, cg::Color{0.4, 0.6, 1.2}), 1.5);
    const std::vector<float3> first = render(64, 36, 4, 0xC0FFEEu, imported, false, &environment);
    const std::vector<float3> second = render(64, 36, 4, 0xC0FFEEu, imported, false, &environment);
    if (first.size() != second.size()) throw std::runtime_error("CUDA image size mismatch");
    float minimum = kInfinity;
    float maximum = 0.0f;
    for (std::size_t i = 0; i < first.size(); ++i) {
        if (first[i].x != second[i].x || first[i].y != second[i].y || first[i].z != second[i].z)
            throw std::runtime_error("CUDA tiled renderer is not deterministic");
        minimum = std::min(minimum, std::min(first[i].x, std::min(first[i].y, first[i].z)));
        maximum = std::max(maximum, std::max(first[i].x, std::max(first[i].y, first[i].z)));
    }
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || maximum - minimum < 0.1f)
        throw std::runtime_error("CUDA render lacks finite image variation");
    std::cout << "CUDA tiled path tracer self-test passed (glTF, HDR, BVH, 64x36, 4 spp, deterministic)\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "--self-test") {
            self_test();
            return 0;
        }
        ImportedAssets imported;
        std::unique_ptr<cg::environment::EnvironmentMap> environment;
        std::string output;
        int samples = 64;
        int width = 640;
        int height = 360;
        Camera camera{make_float3(0.0f, 0.65f, 2.8f),
                      make_float3(0.0f, -0.05f, -4.0f), 48.0f};
        const bool gltf_mode = argc > 1 && std::string(argv[1]) == "--gltf";
        const bool showcase_mode = argc > 1 && std::string(argv[1]) == "--showcase";
        const bool showcase_angle_mode = argc > 1 && std::string(argv[1]) == "--showcase-angle";
        const bool showcase_studio_mode = argc > 1 && std::string(argv[1]) == "--showcase-studio";
        const bool fabric_baseline_mode = argc > 1 && std::string(argv[1]) == "--fabric-baseline";
        const bool fabric_sheen_mode = argc > 1 && std::string(argv[1]) == "--fabric-sheen";
        const bool scene_mode = argc > 1 && std::string(argv[1]) == "--scene";
        const bool hdr_mode = argc > 1 && std::string(argv[1]) == "--hdr";
        if (scene_mode) {
            if (argc < 6) throw std::invalid_argument(
                "usage: cuda_pathtracer --scene output.bmp spp model1.gltf model2.gltf [model3.gltf ...]");
            output = argv[2];
            samples = std::stoi(argv[3]);
            width = 1280;
            height = 720;
            const int model_count = argc - 4;
            for (int model = 0; model < model_count; ++model) {
                ImportedAssets mesh = load_gltf_triangles(argv[model + 4]);
                const float x = (static_cast<float>(model) - 0.5f * static_cast<float>(model_count - 1)) * 1.35f;
                const float z = -4.3f - 0.35f * static_cast<float>(model % 2);
                frame_imported_triangles(mesh.triangles, 1.3f, x, z);
                append_imported(imported, std::move(mesh));
            }
        } else if (gltf_mode || showcase_mode || showcase_angle_mode || showcase_studio_mode ||
                   fabric_baseline_mode || fabric_sheen_mode) {
            if (argc < 3) throw std::invalid_argument(
                "usage: cuda_pathtracer --gltf|--showcase|--showcase-angle|--showcase-studio|--fabric-baseline|--fabric-sheen model.gltf [output.bmp] [spp] [environment.hdr]");
            imported = load_gltf_triangles(argv[2]);
            if (showcase_mode || showcase_angle_mode || showcase_studio_mode) {
                frame_imported_triangles(imported.triangles, 3.8f, 0.0f, -4.5f, 1);
            }
            if (showcase_angle_mode || showcase_studio_mode) {
                camera = {make_float3(2.5f, 0.35f, 1.8f),
                          make_float3(0.0f, -0.15f, -4.35f), 43.0f};
            }
            else if (fabric_baseline_mode || fabric_sheen_mode) {
                frame_imported_triangles(imported.triangles, 3.6f, 0.0f, -4.5f);
                camera = {make_float3(2.3f, 0.55f, 1.7f),
                          make_float3(0.0f, -0.05f, -4.4f), 45.0f};
            }
            else frame_imported_triangles(imported.triangles);
            if (showcase_studio_mode || fabric_baseline_mode)
                for (Triangle& triangle : imported.triangles)
                    triangle.sheen_color = make_float3(0, 0, 0);
            output = argc > 3 ? argv[3] : "cuda_gltf_pathtracer.bmp";
            samples = argc > 4 ? std::stoi(argv[4]) : 64;
            if (argc > 5) environment = std::make_unique<cg::environment::EnvironmentMap>(
                cg::environment::EnvironmentMap::load_radiance(argv[5]));
        } else if (hdr_mode) {
            if (argc < 3) throw std::invalid_argument(
                "usage: cuda_pathtracer --hdr environment.hdr [output.bmp] [spp]");
            environment = std::make_unique<cg::environment::EnvironmentMap>(
                cg::environment::EnvironmentMap::load_radiance(argv[2]));
            output = argc > 3 ? argv[3] : "cuda_hdr_pathtracer.bmp";
            samples = argc > 4 ? std::stoi(argv[4]) : 64;
        } else {
            output = argc > 1 ? argv[1] : "cuda_pathtracer.bmp";
            samples = argc > 2 ? std::stoi(argv[2]) : 64;
        }
        write_image(output, width, height,
                    render(width, height, samples, 0xC0FFEEu, imported,
                           !gltf_mode && !showcase_mode && !showcase_angle_mode &&
                               !showcase_studio_mode && !fabric_baseline_mode &&
                               !fabric_sheen_mode && !scene_mode,
                           environment.get(), camera,
                           showcase_studio_mode || fabric_baseline_mode || fabric_sheen_mode));
        std::cout << "CUDA path traced " << width << 'x' << height << " at " << samples
                  << " spp with " << imported.triangles.size() << " imported triangles"
                  << (environment ? " and HDR environment" : "") << " to " << output << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "CUDA path tracer failed: " << error.what() << '\n';
        return 1;
    }
}
