#include "progressive/progressive_renderer.hpp"

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <vector>

namespace cg::progressive {

Renderer::Renderer(Settings settings) : settings_(settings) {
    if (settings.width == 0 || settings.height == 0 || settings.target_samples == 0 ||
        settings.samples_per_pass == 0 || settings.tile_size == 0 || settings.maximum_depth < 1) {
        throw std::invalid_argument("invalid progressive renderer settings");
    }
    if (settings_.thread_count == 0) {
        settings_.thread_count = std::max(1u, std::thread::hardware_concurrency());
    }
}

Image Renderer::render(const rt::Scene& scene, const rt::Camera& camera, const PassCallback& on_pass) const {
    const std::size_t tiles_x = (settings_.width + settings_.tile_size - 1) / settings_.tile_size;
    const std::size_t tiles_y = (settings_.height + settings_.tile_size - 1) / settings_.tile_size;
    const std::size_t tile_count = tiles_x * tiles_y;
    std::vector<Color> accumulated(settings_.width * settings_.height);
    rt::PathTracer tracer(settings_.width, settings_.height, 1, settings_.maximum_depth, settings_.seed);

    for (std::size_t completed = 0; completed < settings_.target_samples;) {
        const std::size_t pass_samples = std::min(settings_.samples_per_pass, settings_.target_samples - completed);
        std::atomic_size_t next_tile{0};
        std::vector<std::thread> workers;
        workers.reserve(settings_.thread_count);
        for (std::size_t worker = 0; worker < settings_.thread_count; ++worker) {
            workers.emplace_back([&] {
                for (;;) {
                    const std::size_t tile = next_tile.fetch_add(1);
                    if (tile >= tile_count) break;
                    const std::size_t tile_x = tile % tiles_x;
                    const std::size_t tile_y = tile / tiles_x;
                    const std::size_t x_end = std::min((tile_x + 1) * settings_.tile_size, settings_.width);
                    const std::size_t y_end = std::min((tile_y + 1) * settings_.tile_size, settings_.height);
                    for (std::size_t y = tile_y * settings_.tile_size; y < y_end; ++y) {
                        for (std::size_t x = tile_x * settings_.tile_size; x < x_end; ++x) {
                            accumulated[y * settings_.width + x] +=
                                tracer.sample_pixel(scene, camera, x, y, completed, pass_samples) *
                                static_cast<double>(pass_samples);
                        }
                    }
                }
            });
        }
        for (auto& thread : workers) thread.join();
        completed += pass_samples;

        if (on_pass || completed == settings_.target_samples) {
            Image snapshot(settings_.width, settings_.height);
            for (std::size_t y = 0; y < settings_.height; ++y) {
                for (std::size_t x = 0; x < settings_.width; ++x) {
                    snapshot.set(x, y, accumulated[y * settings_.width + x] / static_cast<double>(completed));
                }
            }
            if (on_pass) on_pass(completed, snapshot);
            if (completed == settings_.target_samples) return snapshot;
        }
    }
    throw std::runtime_error("progressive renderer did not produce an image");
}

}  // namespace cg::progressive
