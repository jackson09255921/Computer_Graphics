#include "raytracer/raytracer.hpp"
#include "pbr/pbr.hpp"
#include "sampling/mis.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace cg::rt {
namespace {

constexpr double kRayBias = 1e-5;
constexpr double kInfinity = std::numeric_limits<double>::infinity();

double component(const Vec3& value, int axis) {
    return axis == 0 ? value.x : (axis == 1 ? value.y : value.z);
}

Aabb surrounding(const Aabb& lhs, const Aabb& rhs) {
    return {
        {std::min(lhs.minimum.x, rhs.minimum.x), std::min(lhs.minimum.y, rhs.minimum.y),
         std::min(lhs.minimum.z, rhs.minimum.z)},
        {std::max(lhs.maximum.x, rhs.maximum.x), std::max(lhs.maximum.y, rhs.maximum.y),
         std::max(lhs.maximum.z, rhs.maximum.z)},
    };
}

Vec3 reflect(const Vec3& incident, const Vec3& normal) { return incident - normal * (2.0 * dot(incident, normal)); }

class Sampler {
public:
    explicit Sampler(std::uint64_t seed) : engine_(seed), distribution_(0.0, 1.0) {}
    double next() { return distribution_(engine_); }

private:
    std::mt19937_64 engine_;
    std::uniform_real_distribution<double> distribution_;
};

Vec3 sample_light(const AreaLight& light, Sampler& sampler) {
    return light.center + light.half_u * (2.0 * sampler.next() - 1.0) +
           light.half_v * (2.0 * sampler.next() - 1.0);
}

Vec3 cosine_hemisphere(const Vec3& normal, Sampler& sampler) {
    constexpr double tau = 6.28318530717958647692;
    const double radius = std::sqrt(sampler.next());
    const double angle = tau * sampler.next();
    const double x = radius * std::cos(angle);
    const double y = radius * std::sin(angle);
    const double z = std::sqrt(std::max(0.0, 1.0 - radius * radius));
    const Vec3 helper = std::abs(normal.x) > 0.9 ? Vec3{0.0, 1.0, 0.0} : Vec3{1.0, 0.0, 0.0};
    const Vec3 tangent = normalized(cross(helper, normal));
    const Vec3 bitangent = cross(normal, tangent);
    return normalized(tangent * x + bitangent * y + normal * z);
}

Color sky(const Scene& scene, const Vec3& direction) {
    const double blend = 0.5 * (normalized(direction).y + 1.0);
    return lerp(scene.background, Color{0.12, 0.20, 0.35}, blend);
}

Color path_radiance(const Scene& scene, const Ray& ray, int depth, int maximum_depth, Sampler& sampler) {
    Hit hit;
    if (!scene.intersect(ray, kRayBias, kInfinity, hit)) {
        return sky(scene, ray.direction);
    }
    const Material& material = *hit.material;
    Color result = material.emission;

    for (const AreaLight& light : scene.area_lights()) {
        const Vec3 light_point = sample_light(light, sampler);
        const Vec3 offset = light_point - hit.position;
        const double distance_squared = dot(offset, offset);
        const double distance = std::sqrt(distance_squared);
        const Vec3 direction = offset / distance;
        const double surface_cosine = std::max(0.0, dot(hit.normal, direction));
        const double light_cosine = std::max(0.0, dot(light.normal(), -direction));
        Hit blocker;
        if (surface_cosine > 0.0 && light_cosine > 0.0 &&
            !scene.intersect({hit.position + hit.normal * kRayBias, direction}, kRayBias,
                             distance - kRayBias, blocker)) {
            const Color brdf = pbr::evaluate_ggx(hit.normal, normalized(-ray.direction), direction,
                                                 material.albedo, material.metallic, material.roughness);
            const double light_pdf = distance_squared / (light_cosine * light.area());
            const double bsdf_pdf = sampling::cosine_hemisphere_pdf(surface_cosine);
            const double weight = sampling::power_heuristic(light_pdf, bsdf_pdf);
            result += brdf * light.color *
                      (material.diffuse * light.intensity * surface_cosine * weight / light_pdf);
        }

        const Vec3 bsdf_direction = cosine_hemisphere(hit.normal, sampler);
        sampling::RectangleHit light_hit;
        if (sampling::intersect_area_light(light, hit.position + hit.normal * kRayBias,
                                           bsdf_direction, light_hit)) {
            Hit bsdf_blocker;
            if (!scene.intersect({hit.position + hit.normal * kRayBias, bsdf_direction}, kRayBias,
                                 light_hit.distance - kRayBias, bsdf_blocker)) {
                const double bsdf_surface_cosine = std::max(0.0, dot(hit.normal, bsdf_direction));
                const double bsdf_pdf = sampling::cosine_hemisphere_pdf(bsdf_surface_cosine);
                const double light_pdf = sampling::area_light_pdf(
                    light, hit.position + hit.normal * kRayBias, bsdf_direction);
                const double weight = sampling::power_heuristic(bsdf_pdf, light_pdf);
                const Color brdf = pbr::evaluate_ggx(hit.normal, normalized(-ray.direction), bsdf_direction,
                                                     material.albedo, material.metallic, material.roughness);
                if (bsdf_pdf > 0.0) {
                    result += brdf * light.color *
                              (material.diffuse * light.intensity * bsdf_surface_cosine * weight / bsdf_pdf);
                }
            }
        }
    }

    if (depth >= maximum_depth) {
        return result;
    }
    double survival = std::max({material.albedo.x, material.albedo.y, material.albedo.z});
    survival = clamp(survival, 0.1, 0.95);
    if (depth >= 3 && sampler.next() > survival) {
        return result;
    }
    const double compensation = depth >= 3 ? 1.0 / survival : 1.0;
    Vec3 bounce_direction;
    Color throughput;
    if (material.reflectivity > 0.0 && sampler.next() < material.reflectivity) {
        bounce_direction = normalized(reflect(ray.direction, hit.normal));
        throughput = Color{1.0, 1.0, 1.0};
    } else {
        bounce_direction = cosine_hemisphere(hit.normal, sampler);
        throughput = material.albedo * material.diffuse;
    }
    return result + throughput * path_radiance(scene,
        {hit.position + hit.normal * kRayBias, bounce_direction}, depth + 1, maximum_depth, sampler) * compensation;
}

std::uint64_t mix_seed(std::uint64_t value) {
    value += 0x9E3779B97F4A7C15ull;
    value = (value ^ (value >> 30u)) * 0xBF58476D1CE4E5B9ull;
    value = (value ^ (value >> 27u)) * 0x94D049BB133111EBull;
    return value ^ (value >> 31u);
}

}  // namespace

bool Aabb::intersects(const Ray& ray, double minimum_distance, double maximum_distance) const {
    for (int axis = 0; axis < 3; ++axis) {
        const double direction = component(ray.direction, axis);
        const double origin = component(ray.origin, axis);
        const double slab_minimum = component(minimum, axis);
        const double slab_maximum = component(maximum, axis);
        if (std::abs(direction) <= kEpsilon) {
            if (origin < slab_minimum || origin > slab_maximum) {
                return false;
            }
            continue;
        }
        const double inverse = 1.0 / direction;
        double near_distance = (slab_minimum - origin) * inverse;
        double far_distance = (slab_maximum - origin) * inverse;
        if (inverse < 0.0) {
            std::swap(near_distance, far_distance);
        }
        minimum_distance = std::max(minimum_distance, near_distance);
        maximum_distance = std::min(maximum_distance, far_distance);
        if (maximum_distance < minimum_distance) {
            return false;
        }
    }
    return true;
}

Sphere::Sphere(Vec3 center, double radius, Material material)
    : center_(center), radius_(radius), material_(material) {
    if (radius <= 0.0) {
        throw std::invalid_argument("sphere radius must be positive");
    }
}

Aabb Sphere::bounds() const {
    const Vec3 extent{radius_, radius_, radius_};
    return {center_ - extent, center_ + extent};
}

bool Sphere::intersect(const Ray& ray, double minimum_distance, double maximum_distance, Hit& hit) const {
    const Vec3 offset = ray.origin - center_;
    const double a = dot(ray.direction, ray.direction);
    const double half_b = dot(offset, ray.direction);
    const double c = dot(offset, offset) - radius_ * radius_;
    const double discriminant = half_b * half_b - a * c;
    if (discriminant < 0.0) {
        return false;
    }
    const double root = std::sqrt(discriminant);
    double distance = (-half_b - root) / a;
    if (distance < minimum_distance || distance > maximum_distance) {
        distance = (-half_b + root) / a;
        if (distance < minimum_distance || distance > maximum_distance) {
            return false;
        }
    }
    hit.distance = distance;
    hit.position = ray.at(distance);
    hit.normal = normalized((hit.position - center_) / radius_);
    hit.material = &material_;
    return true;
}

Triangle::Triangle(Vec3 first, Vec3 second, Vec3 third, Material material)
    : first_(first), second_(second), third_(third), normal_(normalized(cross(second - first, third - first))),
      material_(material) {
    if (length(normal_) <= kEpsilon) {
        throw std::invalid_argument("triangle vertices must not be collinear");
    }
}

Aabb Triangle::bounds() const {
    constexpr double padding = 1e-6;
    return {
        {std::min({first_.x, second_.x, third_.x}) - padding, std::min({first_.y, second_.y, third_.y}) - padding,
         std::min({first_.z, second_.z, third_.z}) - padding},
        {std::max({first_.x, second_.x, third_.x}) + padding, std::max({first_.y, second_.y, third_.y}) + padding,
         std::max({first_.z, second_.z, third_.z}) + padding},
    };
}

bool Triangle::intersect(const Ray& ray, double minimum_distance, double maximum_distance, Hit& hit) const {
    const Vec3 edge1 = second_ - first_;
    const Vec3 edge2 = third_ - first_;
    const Vec3 p = cross(ray.direction, edge2);
    const double determinant = dot(edge1, p);
    if (std::abs(determinant) <= kEpsilon) {
        return false;
    }
    const double inverse = 1.0 / determinant;
    const Vec3 offset = ray.origin - first_;
    const double u = dot(offset, p) * inverse;
    if (u < 0.0 || u > 1.0) {
        return false;
    }
    const Vec3 q = cross(offset, edge1);
    const double v = dot(ray.direction, q) * inverse;
    if (v < 0.0 || u + v > 1.0) {
        return false;
    }
    const double distance = dot(edge2, q) * inverse;
    if (distance < minimum_distance || distance > maximum_distance) {
        return false;
    }
    hit.distance = distance;
    hit.position = ray.at(distance);
    hit.normal = dot(normal_, ray.direction) < 0.0 ? normal_ : -normal_;
    hit.material = &material_;
    return true;
}

struct Bvh::Node {
    Aabb bounds{};
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    const Primitive* primitive{nullptr};
};

namespace {

std::unique_ptr<Bvh::Node> build_node(std::vector<const Primitive*>& primitives, std::size_t begin, std::size_t end) {
    auto node = std::make_unique<Bvh::Node>();
    node->bounds = primitives[begin]->bounds();
    for (std::size_t index = begin + 1; index < end; ++index) {
        node->bounds = surrounding(node->bounds, primitives[index]->bounds());
    }
    if (end - begin == 1) {
        node->primitive = primitives[begin];
        return node;
    }

    const Vec3 extent = node->bounds.maximum - node->bounds.minimum;
    const int axis = extent.x > extent.y && extent.x > extent.z ? 0 : (extent.y > extent.z ? 1 : 2);
    const std::size_t middle = begin + (end - begin) / 2;
    std::nth_element(primitives.begin() + static_cast<std::ptrdiff_t>(begin),
                     primitives.begin() + static_cast<std::ptrdiff_t>(middle),
                     primitives.begin() + static_cast<std::ptrdiff_t>(end), [axis](const Primitive* lhs, const Primitive* rhs) {
                         return component(lhs->bounds().centroid(), axis) < component(rhs->bounds().centroid(), axis);
                     });
    node->left = build_node(primitives, begin, middle);
    node->right = build_node(primitives, middle, end);
    return node;
}

bool intersect_node(const Bvh::Node* node, const Ray& ray, double minimum_distance, double maximum_distance, Hit& hit) {
    if (node == nullptr || !node->bounds.intersects(ray, minimum_distance, maximum_distance)) {
        return false;
    }
    if (node->primitive != nullptr) {
        return node->primitive->intersect(ray, minimum_distance, maximum_distance, hit);
    }
    Hit left_hit;
    const bool hit_left = intersect_node(node->left.get(), ray, minimum_distance, maximum_distance, left_hit);
    if (hit_left) {
        maximum_distance = left_hit.distance;
    }
    Hit right_hit;
    const bool hit_right = intersect_node(node->right.get(), ray, minimum_distance, maximum_distance, right_hit);
    hit = hit_right ? right_hit : left_hit;
    return hit_left || hit_right;
}

}  // namespace

Bvh::Bvh(const std::vector<std::shared_ptr<Primitive>>& primitives) {
    if (primitives.empty()) {
        return;
    }
    std::vector<const Primitive*> pointers;
    pointers.reserve(primitives.size());
    for (const auto& primitive : primitives) {
        pointers.push_back(primitive.get());
    }
    root_ = build_node(pointers, 0, pointers.size());
}

Bvh::~Bvh() = default;
Bvh::Bvh(Bvh&&) noexcept = default;
Bvh& Bvh::operator=(Bvh&&) noexcept = default;

bool Bvh::intersect(const Ray& ray, double minimum_distance, double maximum_distance, Hit& hit) const {
    return intersect_node(root_.get(), ray, minimum_distance, maximum_distance, hit);
}

void Scene::add(std::shared_ptr<Primitive> primitive) {
    primitives_.push_back(std::move(primitive));
    bvh_.reset();
}

void Scene::add_light(PointLight light) { lights_.push_back(light); }

Vec3 AreaLight::normal() const { return normalized(cross(half_u, half_v)); }

double AreaLight::area() const { return 4.0 * length(cross(half_u, half_v)); }

void Scene::add_light(AreaLight light) {
    if (light.area() <= kEpsilon || light.intensity < 0.0) {
        throw std::invalid_argument("invalid area light");
    }
    area_lights_.push_back(light);
}

void Scene::build() { bvh_ = std::make_unique<Bvh>(primitives_); }

bool Scene::intersect(const Ray& ray, double minimum_distance, double maximum_distance, Hit& hit) const {
    if (bvh_) {
        return bvh_->intersect(ray, minimum_distance, maximum_distance, hit);
    }
    bool found = false;
    for (const auto& primitive : primitives_) {
        Hit candidate;
        if (primitive->intersect(ray, minimum_distance, maximum_distance, candidate)) {
            found = true;
            maximum_distance = candidate.distance;
            hit = candidate;
        }
    }
    return found;
}

Camera::Camera(Vec3 position, Vec3 target, Vec3 world_up, double vertical_fov_degrees, double aspect_ratio)
    : position_(position), forward_(normalized(target - position)), right_(normalized(cross(forward_, world_up))),
      up_(normalized(cross(right_, forward_))) {
    constexpr double pi = 3.14159265358979323846;
    half_height_ = std::tan(vertical_fov_degrees * pi / 360.0);
    half_width_ = half_height_ * aspect_ratio;
}

Ray Camera::ray(double horizontal, double vertical) const {
    const double x = (2.0 * horizontal - 1.0) * half_width_;
    const double y = (1.0 - 2.0 * vertical) * half_height_;
    return {position_, normalized(forward_ + right_ * x + up_ * y)};
}

Renderer::Renderer(std::size_t width, std::size_t height, int maximum_depth,
                   std::size_t samples_per_pixel, std::size_t shadow_samples, std::uint64_t seed)
    : width_(width), height_(height), maximum_depth_(maximum_depth), samples_per_pixel_(samples_per_pixel),
      shadow_samples_(shadow_samples), seed_(seed) {
    if (width == 0 || height == 0 || maximum_depth < 0 || samples_per_pixel == 0 || shadow_samples == 0) {
        throw std::invalid_argument("invalid renderer configuration");
    }
}

Image Renderer::render(const Scene& scene, const Camera& camera) const {
    Image image(width_, height_);
    Sampler sampler(seed_);
    for (std::size_t y = 0; y < height_; ++y) {
        for (std::size_t x = 0; x < width_; ++x) {
            Color color{};
            for (std::size_t sample = 0; sample < samples_per_pixel_; ++sample) {
                const double jitter_x = samples_per_pixel_ == 1 ? 0.5 : sampler.next();
                const double jitter_y = samples_per_pixel_ == 1 ? 0.5 : sampler.next();
                const double horizontal = (static_cast<double>(x) + jitter_x) / static_cast<double>(width_);
                const double vertical = (static_cast<double>(y) + jitter_y) / static_cast<double>(height_);
                color += trace(scene, camera.ray(horizontal, vertical));
            }
            image.set(x, y, color / static_cast<double>(samples_per_pixel_));
        }
    }
    return image;
}

Color Renderer::trace(const Scene& scene, const Ray& ray, int depth) const {
    Hit hit;
    if (!scene.intersect(ray, kRayBias, kInfinity, hit)) {
        const double blend = 0.5 * (normalized(ray.direction).y + 1.0);
        return lerp(scene.background, Color{0.12, 0.20, 0.35}, blend);
    }

    const Material& material = *hit.material;
    Color result = material.albedo * 0.035;
    const Vec3 view_direction = normalized(-ray.direction);
    for (const PointLight& light : scene.lights()) {
        const Vec3 to_light = light.position - hit.position;
        const double light_distance = length(to_light);
        const Vec3 light_direction = to_light / light_distance;
        Hit blocker;
        const Ray shadow_ray{hit.position + hit.normal * kRayBias, light_direction};
        if (scene.intersect(shadow_ray, kRayBias, light_distance - kRayBias, blocker)) {
            continue;
        }
        const double attenuation = light.intensity / std::max(1.0, light_distance * light_distance);
        const double diffuse = std::max(0.0, dot(hit.normal, light_direction));
        const Vec3 half_vector = normalized(light_direction + view_direction);
        const double specular = std::pow(std::max(0.0, dot(hit.normal, half_vector)), material.shininess);
        result += (material.albedo * (material.diffuse * diffuse) + light.color * (material.specular * specular)) *
                  attenuation * light.color;
    }
    Sampler sampler(seed_ + static_cast<std::uint64_t>(depth));
    for (const AreaLight& light : scene.area_lights()) {
        Color contribution{};
        for (std::size_t sample = 0; sample < shadow_samples_; ++sample) {
            const Vec3 light_position = sample_light(light, sampler);
            const Vec3 to_light = light_position - hit.position;
            const double light_distance = length(to_light);
            const Vec3 light_direction = to_light / light_distance;
            Hit blocker;
            if (scene.intersect({hit.position + hit.normal * kRayBias, light_direction}, kRayBias,
                                light_distance - kRayBias, blocker)) {
                continue;
            }
            const double attenuation = light.intensity / std::max(1.0, light_distance * light_distance);
            const double diffuse = std::max(0.0, dot(hit.normal, light_direction));
            const Vec3 half_vector = normalized(light_direction + view_direction);
            const double specular = std::pow(std::max(0.0, dot(hit.normal, half_vector)), material.shininess);
            contribution += (material.albedo * (material.diffuse * diffuse) +
                             light.color * (material.specular * specular)) * attenuation * light.color;
        }
        result += contribution / static_cast<double>(shadow_samples_);
    }

    if (material.reflectivity > 0.0 && depth < maximum_depth_) {
        const Vec3 reflection_direction = normalized(reflect(ray.direction, hit.normal));
        const Color reflection = trace(scene, {hit.position + hit.normal * kRayBias, reflection_direction}, depth + 1);
        result = lerp(result, reflection, clamp(material.reflectivity));
    }
    return result;
}

PathTracer::PathTracer(std::size_t width, std::size_t height, std::size_t samples_per_pixel,
                       int maximum_depth, std::uint64_t seed)
    : width_(width), height_(height), samples_per_pixel_(samples_per_pixel),
      maximum_depth_(maximum_depth), seed_(seed) {
    if (width == 0 || height == 0 || samples_per_pixel == 0 || maximum_depth < 1) {
        throw std::invalid_argument("invalid path tracer configuration");
    }
}

Image PathTracer::render(const Scene& scene, const Camera& camera) const {
    Image image(width_, height_);
    for (std::size_t y = 0; y < height_; ++y) {
        for (std::size_t x = 0; x < width_; ++x) {
            image.set(x, y, sample_pixel(scene, camera, x, y, 0, samples_per_pixel_));
        }
    }
    return image;
}

Color PathTracer::sample_pixel(const Scene& scene, const Camera& camera, std::size_t x, std::size_t y,
                               std::size_t sample_begin, std::size_t sample_count) const {
    if (x >= width_ || y >= height_ || sample_count == 0) {
        throw std::invalid_argument("invalid path tracer pixel sample range");
    }
    Color color{};
    const std::uint64_t pixel = static_cast<std::uint64_t>(y * width_ + x);
    for (std::size_t offset = 0; offset < sample_count; ++offset) {
        const std::uint64_t sample = static_cast<std::uint64_t>(sample_begin + offset);
        Sampler sampler(mix_seed(seed_ ^ mix_seed(pixel) ^ mix_seed(sample)));
        const double horizontal = (static_cast<double>(x) + sampler.next()) / static_cast<double>(width_);
        const double vertical = (static_cast<double>(y) + sampler.next()) / static_cast<double>(height_);
        color += path_radiance(scene, camera.ray(horizontal, vertical), 0, maximum_depth_, sampler);
    }
    return color / static_cast<double>(sample_count);
}

}  // namespace cg::rt
