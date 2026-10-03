#include "assets/asc_loader.hpp"
#include "raytracer/raytracer.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "usage: legacy_asc_demo input.asc output.(bmp|ppm)\n";
            return 2;
        }
        const cg::assets::AscMesh mesh = cg::assets::AscMesh::load(argv[1]);
        cg::Vec3 minimum{std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                         std::numeric_limits<double>::max()};
        cg::Vec3 maximum{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
                         std::numeric_limits<double>::lowest()};
        for (const cg::Vec3 vertex : mesh.vertices) {
            minimum.x = std::min(minimum.x, vertex.x); minimum.y = std::min(minimum.y, vertex.y);
            minimum.z = std::min(minimum.z, vertex.z); maximum.x = std::max(maximum.x, vertex.x);
            maximum.y = std::max(maximum.y, vertex.y); maximum.z = std::max(maximum.z, vertex.z);
        }
        const cg::Vec3 center = (minimum + maximum) * 0.5;
        const cg::Vec3 extent = maximum - minimum;
        const double radius = std::max({extent.x, extent.y, extent.z, 0.001});
        const cg::rt::Material material{{0.18, 0.52, 0.92}, 0.82, 0.35, 64.0, 0.12};
        cg::rt::Scene scene;
        scene.background = {0.008, 0.012, 0.025};
        for (const cg::assets::AscTriangle triangle : mesh.triangles) {
            scene.add(std::make_shared<cg::rt::Triangle>(mesh.vertices[triangle.first],
                                                         mesh.vertices[triangle.second],
                                                         mesh.vertices[triangle.third], material));
        }
        scene.add_light(cg::rt::PointLight{
            center + cg::Vec3{-1.5 * radius, 1.8 * radius, 2.0 * radius},
            {1.0, 0.86, 0.68}, 5.0 * radius * radius});
        scene.add_light(cg::rt::PointLight{
            center + cg::Vec3{1.4 * radius, 0.4 * radius, 1.2 * radius},
            {0.35, 0.55, 1.0}, 2.0 * radius * radius});
        scene.build();
        constexpr std::size_t width = 512;
        constexpr std::size_t height = 512;
        const cg::Vec3 camera_position = center + cg::Vec3{0.15 * radius, 0.12 * radius, 2.25 * radius};
        const cg::rt::Camera camera(camera_position, center, {0.0, 1.0, 0.0}, 34.0, 1.0);
        const cg::rt::Renderer renderer(width, height, 2, 1, 1, 0xA5C123u);
        const cg::Image image = renderer.render(scene, camera);
        image.write(argv[2]);
        std::cout << "rendered legacy ASC " << mesh.vertices.size() << " vertices / "
                  << mesh.triangles.size() << " triangles to " << argv[2] << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "legacy ASC demo failed: " << error.what() << '\n';
        return 1;
    }
}
