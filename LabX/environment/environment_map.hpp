#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "core/image.hpp"

namespace cg::environment {

class EnvironmentMap {
public:
    EnvironmentMap(std::size_t width, std::size_t height, std::vector<Color> pixels,
                   double intensity = 1.0);

    [[nodiscard]] static EnvironmentMap load_radiance(const std::filesystem::path& path,
                                                       double intensity = 1.0);
    [[nodiscard]] Color sample(const Vec3& direction) const;
    [[nodiscard]] std::size_t width() const noexcept { return width_; }
    [[nodiscard]] std::size_t height() const noexcept { return height_; }

private:
    [[nodiscard]] const Color& pixel(std::size_t x, std::size_t y) const;

    std::size_t width_;
    std::size_t height_;
    std::vector<Color> pixels_;
    double intensity_;
};

}  // namespace cg::environment
