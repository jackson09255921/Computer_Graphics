#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "raytracer/raytracer.hpp"

namespace cg::progressive {

struct Settings {
    std::size_t width{640};
    std::size_t height{360};
    std::size_t target_samples{64};
    std::size_t samples_per_pass{4};
    std::size_t tile_size{16};
    std::size_t thread_count{0};
    int maximum_depth{8};
    std::uint64_t seed{0x5EEDu};
};

using PassCallback = std::function<void(std::size_t completed_samples, const Image& image)>;

class Renderer {
public:
    explicit Renderer(Settings settings);
    [[nodiscard]] Image render(const rt::Scene& scene, const rt::Camera& camera,
                               const PassCallback& on_pass = {}) const;

private:
    Settings settings_;
};

}  // namespace cg::progressive
