#include "environment/environment_map.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace cg::environment {
namespace {

constexpr double kPi = 3.14159265358979323846;

Color decode_rgbe(const std::array<std::uint8_t, 4>& rgbe) {
    if (rgbe[3] == 0) return {};
    const double scale = std::ldexp(1.0, static_cast<int>(rgbe[3]) - (128 + 8));
    return {rgbe[0] * scale, rgbe[1] * scale, rgbe[2] * scale};
}

void read_exact(std::istream& input, std::uint8_t* destination, std::size_t count) {
    input.read(reinterpret_cast<char*>(destination), static_cast<std::streamsize>(count));
    if (!input) throw std::runtime_error("unexpected end of Radiance HDR data");
}

std::vector<Color> read_pixels(std::istream& input, std::size_t width, std::size_t height) {
    std::vector<Color> pixels(width * height);
    for (std::size_t y = 0; y < height; ++y) {
        std::array<std::uint8_t, 4> marker{};
        read_exact(input, marker.data(), marker.size());
        const bool rle = width >= 8 && width <= 32767 && marker[0] == 2 && marker[1] == 2 &&
                         (marker[2] & 0x80u) == 0 &&
                         ((static_cast<std::size_t>(marker[2]) << 8u) | marker[3]) == width;
        if (!rle) {
            pixels[y * width] = decode_rgbe(marker);
            for (std::size_t x = 1; x < width; ++x) {
                std::array<std::uint8_t, 4> rgbe{};
                read_exact(input, rgbe.data(), rgbe.size());
                pixels[y * width + x] = decode_rgbe(rgbe);
            }
            continue;
        }

        std::vector<std::uint8_t> channels(width * 4);
        for (std::size_t channel = 0; channel < 4; ++channel) {
            std::size_t x = 0;
            while (x < width) {
                std::uint8_t code = 0;
                read_exact(input, &code, 1);
                if (code > 128) {
                    const std::size_t run = code - 128;
                    std::uint8_t value = 0;
                    read_exact(input, &value, 1);
                    if (run == 0 || x + run > width) throw std::runtime_error("invalid Radiance HDR RLE run");
                    std::fill_n(channels.begin() + static_cast<std::ptrdiff_t>(channel * width + x), run, value);
                    x += run;
                } else {
                    const std::size_t run = code;
                    if (run == 0 || x + run > width) throw std::runtime_error("invalid Radiance HDR RLE literal");
                    read_exact(input, channels.data() + channel * width + x, run);
                    x += run;
                }
            }
        }
        for (std::size_t x = 0; x < width; ++x) {
            pixels[y * width + x] = decode_rgbe({channels[x], channels[width + x],
                                                  channels[2 * width + x], channels[3 * width + x]});
        }
    }
    return pixels;
}

}  // namespace

EnvironmentMap::EnvironmentMap(std::size_t width, std::size_t height, std::vector<Color> pixels,
                               double intensity)
    : width_(width), height_(height), pixels_(std::move(pixels)), intensity_(intensity) {
    if (width == 0 || height == 0 || pixels_.size() != width * height || intensity < 0.0) {
        throw std::invalid_argument("invalid environment map dimensions, pixels, or intensity");
    }
}

EnvironmentMap EnvironmentMap::load_radiance(const std::filesystem::path& path, double intensity) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("failed to open Radiance HDR image: " + path.string());
    std::string line;
    std::getline(input, line);
    if (line.rfind("#?RADIANCE", 0) != 0 && line.rfind("#?RGBE", 0) != 0) {
        throw std::runtime_error("not a Radiance RGBE image: " + path.string());
    }
    while (std::getline(input, line) && !line.empty() && line != "\r") {}
    if (!std::getline(input, line)) throw std::runtime_error("Radiance HDR image has no resolution line");
    std::istringstream resolution(line);
    std::string y_axis;
    std::string x_axis;
    std::size_t height = 0;
    std::size_t width = 0;
    resolution >> y_axis >> height >> x_axis >> width;
    if (!resolution || y_axis != "-Y" || x_axis != "+X" || width == 0 || height == 0) {
        throw std::runtime_error("only top-to-bottom -Y +X Radiance HDR images are supported");
    }
    return {width, height, read_pixels(input, width, height), intensity};
}

const Color& EnvironmentMap::pixel(std::size_t x, std::size_t y) const {
    return pixels_[y * width_ + (x % width_)];
}

Color EnvironmentMap::sample(const Vec3& direction) const {
    const Vec3 unit = normalized(direction);
    if (length(unit) <= kEpsilon) return {};
    const double u = std::atan2(unit.z, unit.x) / (2.0 * kPi) + 0.5;
    const double v = std::acos(clamp(unit.y, -1.0, 1.0)) / kPi;
    const double x = u * static_cast<double>(width_) - 0.5;
    const double y = v * static_cast<double>(height_) - 0.5;
    const auto x0_signed = static_cast<long long>(std::floor(x));
    const std::size_t x0 = static_cast<std::size_t>((x0_signed % static_cast<long long>(width_) +
                                                     static_cast<long long>(width_)) % static_cast<long long>(width_));
    const std::size_t x1 = (x0 + 1) % width_;
    const std::size_t y0 = static_cast<std::size_t>(clamp(std::floor(y), 0.0, static_cast<double>(height_ - 1)));
    const std::size_t y1 = std::min(y0 + 1, height_ - 1);
    const double tx = x - std::floor(x);
    const double ty = clamp(y - std::floor(y));
    return lerp(lerp(pixel(x0, y0), pixel(x1, y0), tx),
                lerp(pixel(x0, y1), pixel(x1, y1), tx), ty) * intensity_;
}

}  // namespace cg::environment
