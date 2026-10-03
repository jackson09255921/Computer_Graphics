#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "core/math.hpp"

namespace cg {

using Color = Vec3;

class Image {
public:
    Image(std::size_t width, std::size_t height, Color clear_color = {});

    [[nodiscard]] std::size_t width() const noexcept { return width_; }
    [[nodiscard]] std::size_t height() const noexcept { return height_; }

    void set(std::size_t x, std::size_t y, Color color);
    [[nodiscard]] const Color& get(std::size_t x, std::size_t y) const;
    void write_ppm(const std::filesystem::path& path) const;

private:
    [[nodiscard]] std::size_t index(std::size_t x, std::size_t y) const;

    std::size_t width_;
    std::size_t height_;
    std::vector<Color> pixels_;
};

}  // namespace cg
