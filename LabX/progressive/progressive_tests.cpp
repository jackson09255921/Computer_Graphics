#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "progressive/progressive_renderer.hpp"

int main() {
    try {
        cg::rt::Scene scene;
        scene.add(std::make_shared<cg::rt::Sphere>(cg::Vec3{0.0, 0.0, -3.0}, 0.8, cg::rt::Material{}));
        scene.add_light(cg::rt::AreaLight{{0.0, 3.0, -2.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
                                          {1.0, 1.0, 1.0}, 5.0});
        scene.build();
        const cg::rt::Camera camera({0.0, 0.0, 0.0}, {0.0, 0.0, -3.0}, {0.0, 1.0, 0.0}, 45.0, 1.0);

        cg::progressive::Settings settings{8, 8, 4, 2, 3, 1, 4, 1234};
        std::vector<std::size_t> passes;
        const cg::Image serial = cg::progressive::Renderer(settings).render(
            scene, camera, [&](std::size_t samples, const cg::Image&) { passes.push_back(samples); });
        if (passes != std::vector<std::size_t>{2, 4}) throw std::runtime_error("unexpected pass sequence");

        settings.thread_count = 4;
        const cg::Image parallel = cg::progressive::Renderer(settings).render(scene, camera);
        for (std::size_t y = 0; y < 8; ++y) {
            for (std::size_t x = 0; x < 8; ++x) {
                const cg::Color delta = serial.get(x, y) - parallel.get(x, y);
                if (cg::length(delta) > 1e-12) throw std::runtime_error("thread count changed deterministic output");
            }
        }
        std::cout << "progressive renderer tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "progressive renderer tests failed: " << error.what() << '\n';
        return 1;
    }
}
