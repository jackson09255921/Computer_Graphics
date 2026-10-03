#include "core/image.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>

namespace cg {
namespace {

std::uint8_t to_srgb_byte(double linear) {
    const double value = clamp(linear);
    const double srgb = value <= 0.0031308 ? 12.92 * value : 1.055 * std::pow(value, 1.0 / 2.4) - 0.055;
    return static_cast<std::uint8_t>(std::lround(clamp(srgb) * 255.0));
}

}  // namespace

Image::Image(std::size_t width, std::size_t height, Color clear_color)
    : width_(width), height_(height), pixels_(width * height, clear_color) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("image dimensions must be greater than zero");
    }
}

std::size_t Image::index(std::size_t x, std::size_t y) const {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("pixel coordinate is outside the image");
    }
    return y * width_ + x;
}

void Image::set(std::size_t x, std::size_t y, Color color) { pixels_[index(x, y)] = color; }

const Color& Image::get(std::size_t x, std::size_t y) const { return pixels_[index(x, y)]; }

void Image::write_ppm(const std::filesystem::path& path) const {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to open output image: " + path.string());
    }
    output << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    for (const Color& color : pixels_) {
        const std::uint8_t bytes[] = {to_srgb_byte(color.x), to_srgb_byte(color.y), to_srgb_byte(color.z)};
        output.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    }
}

}  // namespace cg
