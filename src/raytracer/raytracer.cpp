#include "raytracer/raytracer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
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

Renderer::Renderer(std::size_t width, std::size_t height, int maximum_depth)
    : width_(width), height_(height), maximum_depth_(maximum_depth) {
    if (width == 0 || height == 0 || maximum_depth < 0) {
        throw std::invalid_argument("invalid renderer configuration");
    }
}

Image Renderer::render(const Scene& scene, const Camera& camera) const {
    Image image(width_, height_);
    for (std::size_t y = 0; y < height_; ++y) {
        for (std::size_t x = 0; x < width_; ++x) {
            const double horizontal = (static_cast<double>(x) + 0.5) / static_cast<double>(width_);
            const double vertical = (static_cast<double>(y) + 0.5) / static_cast<double>(height_);
            image.set(x, y, trace(scene, camera.ray(horizontal, vertical)));
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

    if (material.reflectivity > 0.0 && depth < maximum_depth_) {
        const Vec3 reflection_direction = normalized(reflect(ray.direction, hit.normal));
        const Color reflection = trace(scene, {hit.position + hit.normal * kRayBias, reflection_direction}, depth + 1);
        result = lerp(result, reflection, clamp(material.reflectivity));
    }
    return result;
}

}  // namespace cg::rt
