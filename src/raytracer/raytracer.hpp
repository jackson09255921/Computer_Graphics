#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/image.hpp"
#include "core/math.hpp"

namespace cg::rt {

struct Ray {
    Vec3 origin;
    Vec3 direction;

    [[nodiscard]] Vec3 at(double distance) const { return origin + direction * distance; }
};

struct Material {
    Color albedo{0.8, 0.8, 0.8};
    double diffuse{0.8};
    double specular{0.2};
    double shininess{32.0};
    double reflectivity{0.0};
};

struct Aabb {
    Vec3 minimum;
    Vec3 maximum;

    [[nodiscard]] bool intersects(const Ray& ray, double minimum_distance, double maximum_distance) const;
    [[nodiscard]] Vec3 centroid() const { return (minimum + maximum) * 0.5; }
};

struct Hit {
    double distance{0.0};
    Vec3 position;
    Vec3 normal;
    const Material* material{nullptr};
};

class Primitive {
public:
    virtual ~Primitive() = default;
    [[nodiscard]] virtual Aabb bounds() const = 0;
    [[nodiscard]] virtual bool intersect(const Ray& ray, double minimum_distance, double maximum_distance,
                                         Hit& hit) const = 0;
};

class Sphere final : public Primitive {
public:
    Sphere(Vec3 center, double radius, Material material);
    [[nodiscard]] Aabb bounds() const override;
    [[nodiscard]] bool intersect(const Ray& ray, double minimum_distance, double maximum_distance,
                                 Hit& hit) const override;

private:
    Vec3 center_;
    double radius_;
    Material material_;
};

class Triangle final : public Primitive {
public:
    Triangle(Vec3 first, Vec3 second, Vec3 third, Material material);
    [[nodiscard]] Aabb bounds() const override;
    [[nodiscard]] bool intersect(const Ray& ray, double minimum_distance, double maximum_distance,
                                 Hit& hit) const override;

private:
    Vec3 first_;
    Vec3 second_;
    Vec3 third_;
    Vec3 normal_;
    Material material_;
};

struct PointLight {
    Vec3 position;
    Color color{1.0, 1.0, 1.0};
    double intensity{1.0};
};

class Bvh {
public:
    struct Node;

    explicit Bvh(const std::vector<std::shared_ptr<Primitive>>& primitives);
    ~Bvh();
    Bvh(Bvh&&) noexcept;
    Bvh& operator=(Bvh&&) noexcept;
    Bvh(const Bvh&) = delete;
    Bvh& operator=(const Bvh&) = delete;

    [[nodiscard]] bool intersect(const Ray& ray, double minimum_distance, double maximum_distance, Hit& hit) const;

private:
    std::unique_ptr<Node> root_;
};

class Scene {
public:
    void add(std::shared_ptr<Primitive> primitive);
    void add_light(PointLight light);
    void build();

    [[nodiscard]] bool intersect(const Ray& ray, double minimum_distance, double maximum_distance, Hit& hit) const;
    [[nodiscard]] const std::vector<PointLight>& lights() const noexcept { return lights_; }

    Color background{0.015, 0.025, 0.06};

private:
    std::vector<std::shared_ptr<Primitive>> primitives_;
    std::vector<PointLight> lights_;
    std::unique_ptr<Bvh> bvh_;
};

class Camera {
public:
    Camera(Vec3 position, Vec3 target, Vec3 world_up, double vertical_fov_degrees, double aspect_ratio);
    [[nodiscard]] Ray ray(double horizontal, double vertical) const;

private:
    Vec3 position_;
    Vec3 forward_;
    Vec3 right_;
    Vec3 up_;
    double half_width_;
    double half_height_;
};

class Renderer {
public:
    Renderer(std::size_t width, std::size_t height, int maximum_depth = 3);
    [[nodiscard]] Image render(const Scene& scene, const Camera& camera) const;
    [[nodiscard]] Color trace(const Scene& scene, const Ray& ray, int depth = 0) const;

private:
    std::size_t width_;
    std::size_t height_;
    int maximum_depth_;
};

}  // namespace cg::rt
