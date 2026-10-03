#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

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
struct Sphere { float3 center; float radius; float3 albedo; float metallic; };
struct Triangle { float3 first; float3 second; float3 third; float3 albedo; float metallic; };
struct BvhNode { float3 minimum; float3 maximum; int left; int right; int first; int count; };
struct Hit { float distance; float3 position; float3 normal; float3 albedo; float metallic; bool found; };

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
                closest.normal = normalize3(cross3(edge1, edge2));
                if (dot3(closest.normal, ray.direction) > 0.0f) closest.normal = mul(closest.normal, -1.0f);
                closest.albedo = triangle.albedo;
                closest.metallic = triangle.metallic;
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
        if (dot3(closest.normal, ray.direction) > 0.0f) closest.normal = mul(closest.normal, -1.0f);
        closest.albedo = sphere.albedo;
        closest.metallic = sphere.metallic;
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

__device__ float3 radiance(Ray ray, const Sphere* spheres, int sphere_count,
                           const Triangle* triangles, const BvhNode* nodes, int node_count, Rng& rng) {
    float3 result = make_float3(0, 0, 0);
    float3 throughput = make_float3(1, 1, 1);
    for (int depth = 0; depth < 6; ++depth) {
        Hit hit{};
        if (!intersect_scene(ray, spheres, sphere_count, triangles, nodes, node_count, hit)) {
            const float blend = 0.5f * (ray.direction.y + 1.0f);
            const float3 sky = add(mul(make_float3(0.02f, 0.03f, 0.06f), 1.0f - blend),
                                   mul(make_float3(0.35f, 0.55f, 0.9f), blend));
            result = add(result, mul(throughput, sky));
            break;
        }
        throughput = mul(throughput, hit.albedo);
        float3 direction;
        if (hit.metallic > 0.5f) {
            direction = reflect3(ray.direction, hit.normal);
            direction = normalize3(add(direction, mul(cosine_direction(hit.normal, rng), 0.06f)));
        } else {
            direction = cosine_direction(hit.normal, rng);
        }
        ray = {add(hit.position, mul(hit.normal, 1.0e-4f)), direction};
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
                            const BvhNode* nodes, int node_count) {
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
        const float3 origin = make_float3(0.0f, 0.65f, 2.8f);
        const float3 target = make_float3(0.0f, -0.05f, -4.0f);
        const float3 forward = normalize3(sub(target, origin));
        const float3 right = normalize3(cross3(forward, make_float3(0, 1, 0)));
        const float3 up = cross3(right, forward);
        const float viewport = tanf(48.0f * kPi / 360.0f) * 2.0f;
        const float3 direction = normalize3(add(forward, add(mul(right, (u - 0.5f) * viewport * aspect),
                                                               mul(up, (0.5f - v) * viewport))));
        color = add(color, radiance({origin, direction}, spheres, sphere_count,
                                    triangles, nodes, node_count, rng));
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
         make_float3(1, 1, 1), 0.0f},
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

std::vector<float3> render(int width, int height, int samples, std::uint32_t seed) {
    const std::vector<Sphere> spheres{
        {make_float3(-1.15f, -0.05f, -4.2f), 0.95f, make_float3(0.85f, 0.12f, 0.06f), 0.0f},
        {make_float3(1.05f, -0.15f, -3.8f), 0.85f, make_float3(0.88f, 0.9f, 0.95f), 1.0f},
    };
    std::vector<Triangle> triangles{
        {make_float3(-7, -1, 2), make_float3(7, -1, 2), make_float3(7, -1, -12),
         make_float3(0.65f, 0.68f, 0.72f), 0.0f},
        {make_float3(-7, -1, 2), make_float3(7, -1, -12), make_float3(-7, -1, -12),
         make_float3(0.65f, 0.68f, 0.72f), 0.0f},
        {make_float3(-7, -1, -9), make_float3(7, -1, -9), make_float3(7, 6, -9),
         make_float3(0.3f, 0.38f, 0.5f), 0.0f},
        {make_float3(-7, -1, -9), make_float3(7, 6, -9), make_float3(-7, 6, -9),
         make_float3(0.3f, 0.38f, 0.5f), 0.0f},
    };
    const std::vector<BvhNode> nodes = build_bvh(triangles);
    Sphere* device_spheres = nullptr;
    Triangle* device_triangles = nullptr;
    BvhNode* device_nodes = nullptr;
    float3* device_tile = nullptr;
    check(cudaMalloc(&device_spheres, spheres.size() * sizeof(Sphere)), "cudaMalloc spheres");
    try {
        check(cudaMalloc(&device_triangles, triangles.size() * sizeof(Triangle)), "cudaMalloc triangles");
        check(cudaMalloc(&device_nodes, nodes.size() * sizeof(BvhNode)), "cudaMalloc BVH nodes");
        check(cudaMalloc(&device_tile, kTileWidth * kTileHeight * sizeof(float3)), "cudaMalloc tile");
        check(cudaMemcpy(device_spheres, spheres.data(), spheres.size() * sizeof(Sphere), cudaMemcpyHostToDevice),
              "copy spheres");
        check(cudaMemcpy(device_triangles, triangles.data(), triangles.size() * sizeof(Triangle), cudaMemcpyHostToDevice),
              "copy triangles");
        check(cudaMemcpy(device_nodes, nodes.data(), nodes.size() * sizeof(BvhNode), cudaMemcpyHostToDevice),
              "copy BVH nodes");
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
                                                  device_nodes, static_cast<int>(nodes.size()));
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
        check(cudaFree(device_nodes), "cudaFree BVH nodes");
        check(cudaFree(device_triangles), "cudaFree triangles");
        check(cudaFree(device_spheres), "cudaFree spheres");
        return image;
    } catch (...) {
        if (device_tile) cudaFree(device_tile);
        if (device_nodes) cudaFree(device_nodes);
        if (device_triangles) cudaFree(device_triangles);
        cudaFree(device_spheres);
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
    const std::vector<float3> first = render(64, 36, 4, 0xC0FFEEu);
    const std::vector<float3> second = render(64, 36, 4, 0xC0FFEEu);
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
    std::cout << "CUDA tiled path tracer self-test passed (64x36, 4 spp, deterministic)\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "--self-test") {
            self_test();
            return 0;
        }
        const std::string output = argc > 1 ? argv[1] : "cuda_pathtracer.bmp";
        const int samples = argc > 2 ? std::stoi(argv[2]) : 64;
        const int width = 640;
        const int height = 360;
        write_image(output, width, height, render(width, height, samples, 0xC0FFEEu));
        std::cout << "CUDA path traced " << width << 'x' << height << " at " << samples
                  << " spp to " << output << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "CUDA path tracer failed: " << error.what() << '\n';
        return 1;
    }
}
