#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace cg::visual {

struct Image {
    std::size_t width{};
    std::size_t height{};
    std::vector<std::uint8_t> rgb;

    static Image read(const std::filesystem::path& path);
    void write_ppm(const std::filesystem::path& path) const;
};

struct Metrics {
    double mae{};
    double rmse{};
    std::uint8_t maximum_error{};
    std::size_t changed_channels{};
};

Metrics compare(const Image& reference, const Image& actual);
Image difference_heatmap(const Image& reference, const Image& actual, unsigned int amplification = 4);

}  // namespace cg::visual
