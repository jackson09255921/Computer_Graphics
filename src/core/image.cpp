#include "core/image.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <type_traits>

namespace cg {
namespace {

std::uint8_t to_srgb_byte(double linear) {
    const double value = clamp(linear);
    const double srgb = value <= 0.0031308 ? 12.92 * value : 1.055 * std::pow(value, 1.0 / 2.4) - 0.055;
    return static_cast<std::uint8_t>(std::lround(clamp(srgb) * 255.0));
}

template <typename Integer>
void write_little_endian(std::ostream& output, Integer value) {
    using Unsigned = std::make_unsigned_t<Integer>;
    const Unsigned bits = static_cast<Unsigned>(value);
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        output.put(static_cast<char>((bits >> (index * 8U)) & 0xffU));
    }
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

void Image::write(const std::filesystem::path& path) const {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    if (extension == ".bmp") {
        write_bmp(path);
    } else if (extension == ".ppm") {
        write_ppm(path);
    } else {
        throw std::invalid_argument("supported image extensions are .bmp and .ppm");
    }
}

void Image::write_bmp(const std::filesystem::path& path) const {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to open output image: " + path.string());
    }
    const std::uint32_t row_size = static_cast<std::uint32_t>((width_ * 3U + 3U) & ~3U);
    const std::uint32_t pixel_bytes = row_size * static_cast<std::uint32_t>(height_);
    const std::uint32_t data_offset = 14U + 40U;

    output.write("BM", 2);
    write_little_endian(output, data_offset + pixel_bytes);
    write_little_endian(output, std::uint32_t{0});
    write_little_endian(output, data_offset);
    write_little_endian(output, std::uint32_t{40});
    write_little_endian(output, static_cast<std::int32_t>(width_));
    write_little_endian(output, -static_cast<std::int32_t>(height_));
    write_little_endian(output, std::uint16_t{1});
    write_little_endian(output, std::uint16_t{24});
    write_little_endian(output, std::uint32_t{0});
    write_little_endian(output, pixel_bytes);
    write_little_endian(output, std::int32_t{2835});
    write_little_endian(output, std::int32_t{2835});
    write_little_endian(output, std::uint32_t{0});
    write_little_endian(output, std::uint32_t{0});

    const std::array<char, 3> padding{};
    for (std::size_t y = 0; y < height_; ++y) {
        for (std::size_t x = 0; x < width_; ++x) {
            const Color& color = get(x, y);
            const std::uint8_t bytes[] = {to_srgb_byte(color.z), to_srgb_byte(color.y), to_srgb_byte(color.x)};
            output.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
        }
        output.write(padding.data(), static_cast<std::streamsize>(row_size - width_ * 3U));
    }
}

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
