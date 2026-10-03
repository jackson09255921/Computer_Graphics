#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "raytracer/raytracer.hpp"

namespace {

bool near(double lhs, double rhs, double tolerance = 1e-8) { return std::abs(lhs - rhs) <= tolerance; }

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

}  // namespace

int main() {
    try {
        const cg::rt::Material material;
        const cg::rt::Ray center_ray{{0.0, 0.0, 0.0}, {0.0, 0.0, -1.0}};

        const cg::rt::Sphere sphere({0.0, 0.0, -5.0}, 1.0, material);
        cg::rt::Hit sphere_hit;
        require(sphere.intersect(center_ray, 1e-5, 100.0, sphere_hit), "center ray should hit sphere");
        require(near(sphere_hit.distance, 4.0), "sphere should be four units from the ray origin");

        const cg::rt::Triangle triangle({-1.0, -1.0, -3.0}, {1.0, -1.0, -3.0}, {0.0, 1.0, -3.0}, material);
        cg::rt::Hit triangle_hit;
        require(triangle.intersect(center_ray, 1e-5, 100.0, triangle_hit), "center ray should hit triangle");
        require(near(triangle_hit.distance, 3.0), "triangle should be three units from the ray origin");

        cg::rt::Scene scene;
        scene.add(std::make_shared<cg::rt::Sphere>(cg::Vec3{0.0, 0.0, -5.0}, 1.0, material));
        scene.add(std::make_shared<cg::rt::Triangle>(cg::Vec3{-1.0, -1.0, -3.0}, cg::Vec3{1.0, -1.0, -3.0},
                                                    cg::Vec3{0.0, 1.0, -3.0}, material));
        scene.build();
        cg::rt::Hit bvh_hit;
        require(scene.intersect(center_ray, 1e-5, 100.0, bvh_hit), "BVH should report the nearest hit");
        require(near(bvh_hit.distance, 3.0), "BVH should return the triangle before the sphere");

        std::cout << "ray tracer tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ray tracer tests failed: " << error.what() << '\n';
        return 1;
    }
}
