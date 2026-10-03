#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>

#include "raytracer/raytracer.hpp"

int main(int argc, char** argv) {
    try {
        const std::filesystem::path output = argc > 1 ? argv[1] : "raytracer_demo.bmp";
        constexpr std::size_t width = 800;
        constexpr std::size_t height = 450;

        const cg::rt::Material red{{0.9, 0.08, 0.05}, 0.85, 0.25, 64.0, 0.12};
        const cg::rt::Material blue{{0.04, 0.18, 0.95}, 0.75, 0.5, 96.0, 0.45};
        const cg::rt::Material gold{{0.95, 0.55, 0.08}, 0.8, 0.5, 80.0, 0.2};
        const cg::rt::Material floor{{0.32, 0.36, 0.42}, 0.9, 0.08, 16.0, 0.08};

        cg::rt::Scene scene;
        scene.add(std::make_shared<cg::rt::Sphere>(cg::Vec3{-1.25, 0.0, -4.2}, 1.0, red));
        scene.add(std::make_shared<cg::rt::Sphere>(cg::Vec3{1.15, -0.15, -3.5}, 0.85, blue));
        scene.add(std::make_shared<cg::rt::Sphere>(cg::Vec3{0.1, 1.45, -5.0}, 0.65, gold));
        scene.add(std::make_shared<cg::rt::Triangle>(cg::Vec3{-7.0, -1.05, 1.0}, cg::Vec3{7.0, -1.05, 1.0},
                                                    cg::Vec3{7.0, -1.05, -12.0}, floor));
        scene.add(std::make_shared<cg::rt::Triangle>(cg::Vec3{-7.0, -1.05, 1.0}, cg::Vec3{7.0, -1.05, -12.0},
                                                    cg::Vec3{-7.0, -1.05, -12.0}, floor));
        scene.add_light({{-3.5, 5.0, 1.0}, {1.0, 0.88, 0.72}, 65.0});
        scene.add_light({{4.0, 2.5, -1.0}, {0.35, 0.55, 1.0}, 28.0});
        scene.build();

        const cg::rt::Camera camera({0.0, 1.0, 2.8}, {0.0, 0.1, -4.0}, {0.0, 1.0, 0.0}, 48.0,
                                    static_cast<double>(width) / static_cast<double>(height));
        const cg::rt::Renderer renderer(width, height, 4);
        const auto start = std::chrono::steady_clock::now();
        const cg::Image image = renderer.render(scene, camera);
        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        image.write(output);
        std::cout << "rendered " << width << 'x' << height << " image to " << output.string() << " in " << elapsed
                  << " seconds\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ray tracer failed: " << error.what() << '\n';
        return 1;
    }
}
