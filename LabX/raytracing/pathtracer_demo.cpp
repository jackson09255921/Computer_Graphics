#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "raytracer/raytracer.hpp"
#include "progressive/progressive_renderer.hpp"

int main(int argc, char** argv) {
    try {
        const std::filesystem::path output = argc > 1 ? argv[1] : "pathtracer_demo.bmp";
        const std::size_t samples = argc > 2 ? static_cast<std::size_t>(std::stoul(argv[2])) : 64;
        constexpr std::size_t width = 640;
        constexpr std::size_t height = 360;

        const cg::rt::Material coral{{0.85, 0.12, 0.06}, 0.9, 0.0, 1.0, 0.0};
        cg::rt::Material blue_glass;
        blue_glass.albedo = {0.72, 0.88, 1.0};
        blue_glass.diffuse = 0.03;
        blue_glass.specular = 1.0;
        blue_glass.roughness = 0.16;
        blue_glass.transmission = 0.92;
        blue_glass.index_of_refraction = 1.5;
        const cg::rt::Material mirror{{0.92, 0.92, 0.92}, 0.0, 0.0, 1.0, 0.92};
        const cg::rt::Material floor{{0.62, 0.64, 0.68}, 0.9, 0.0, 1.0, 0.0};

        cg::rt::Scene scene;
        scene.background = {0.004, 0.006, 0.012};
        scene.add(std::make_shared<cg::rt::Sphere>(cg::Vec3{-1.25, -0.05, -4.3}, 1.0, coral));
        scene.add(std::make_shared<cg::rt::Sphere>(cg::Vec3{1.1, -0.2, -3.7}, 0.85, mirror));
        scene.add(std::make_shared<cg::rt::Sphere>(cg::Vec3{0.2, 1.25, -5.1}, 0.62, blue_glass));
        scene.add(std::make_shared<cg::rt::Triangle>(cg::Vec3{-7.0, -1.05, 1.0}, cg::Vec3{7.0, -1.05, 1.0},
                                                    cg::Vec3{7.0, -1.05, -12.0}, floor));
        scene.add(std::make_shared<cg::rt::Triangle>(cg::Vec3{-7.0, -1.05, 1.0}, cg::Vec3{7.0, -1.05, -12.0},
                                                    cg::Vec3{-7.0, -1.05, -12.0}, floor));
        scene.add_light(cg::rt::AreaLight{{-1.5, 4.5, -2.0}, {1.4, 0.0, 0.0}, {0.0, 0.0, 1.0},
                                          {1.0, 0.82, 0.62}, 12.0});
        scene.build();

        const cg::rt::Camera camera({0.0, 0.9, 2.8}, {0.0, 0.0, -4.1}, {0.0, 1.0, 0.0}, 48.0,
                                    static_cast<double>(width) / static_cast<double>(height));
        const cg::progressive::Renderer renderer({width, height, samples, std::min<std::size_t>(4, samples),
                                                   16, 0, 8, 0x5EEDu});
        const auto start = std::chrono::steady_clock::now();
        const cg::Image image = renderer.render(scene, camera);
        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        image.write(output);
        std::cout << "path traced " << width << 'x' << height << " at " << samples << " spp to "
                  << output.string() << " in " << elapsed << " seconds\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "path tracer failed: " << error.what() << '\n';
        return 1;
    }
}
