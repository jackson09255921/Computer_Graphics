#include "visual/visual_regression.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace cg::visual {
namespace {

std::uint16_t read_u16(std::istream& input) {
    const unsigned int low = static_cast<unsigned char>(input.get());
    const unsigned int high = static_cast<unsigned char>(input.get());
    return static_cast<std::uint16_t>(low | (high << 8U));
}

std::uint32_t read_u32(std::istream& input) {
    std::uint32_t value = 0;
    for (unsigned int byte = 0; byte < 4; ++byte)
        value |= static_cast<std::uint32_t>(static_cast<unsigned char>(input.get())) << (byte * 8U);
    return value;
}

std::string ppm_token(std::istream& input) {
    std::string token;
    while (input) {
        const int character = input.peek();
        if (character == '#') {
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
            input.get();
        } else {
            break;
        }
    }
    input >> token;
    return token;
}

Image read_ppm(std::istream& input) {
    if (ppm_token(input) != "P6") throw std::runtime_error("only binary P6 PPM is supported");
    const std::size_t width = static_cast<std::size_t>(std::stoull(ppm_token(input)));
    const std::size_t height = static_cast<std::size_t>(std::stoull(ppm_token(input)));
    if (ppm_token(input) != "255") throw std::runtime_error("PPM maximum value must be 255");
    input.get();
    Image image{width, height, std::vector<std::uint8_t>(width * height * 3U)};
    input.read(reinterpret_cast<char*>(image.rgb.data()), static_cast<std::streamsize>(image.rgb.size()));
    if (input.gcount() != static_cast<std::streamsize>(image.rgb.size()))
        throw std::runtime_error("truncated PPM pixel data");
    return image;
}

Image read_bmp(std::istream& input) {
    if (input.get() != 'B' || input.get() != 'M') throw std::runtime_error("invalid BMP signature");
    (void)read_u32(input);
    (void)read_u32(input);
    const std::uint32_t pixel_offset = read_u32(input);
    if (read_u32(input) != 40U) throw std::runtime_error("only BITMAPINFOHEADER BMP is supported");
    const std::int32_t signed_width = static_cast<std::int32_t>(read_u32(input));
    const std::int32_t signed_height = static_cast<std::int32_t>(read_u32(input));
    if (signed_width <= 0 || signed_height == 0) throw std::runtime_error("invalid BMP dimensions");
    if (read_u16(input) != 1U || read_u16(input) != 24U || read_u32(input) != 0U)
        throw std::runtime_error("only uncompressed 24-bit BMP is supported");
    const std::size_t width = static_cast<std::size_t>(signed_width);
    const std::size_t height = static_cast<std::size_t>(std::abs(signed_height));
    const std::size_t row_size = (width * 3U + 3U) & ~3U;
    Image image{width, height, std::vector<std::uint8_t>(width * height * 3U)};
    input.seekg(pixel_offset, std::ios::beg);
    std::vector<std::uint8_t> row(row_size);
    for (std::size_t file_y = 0; file_y < height; ++file_y) {
        input.read(reinterpret_cast<char*>(row.data()), static_cast<std::streamsize>(row.size()));
        if (!input) throw std::runtime_error("truncated BMP pixel data");
        const std::size_t y = signed_height < 0 ? file_y : height - 1U - file_y;
        for (std::size_t x = 0; x < width; ++x) {
            const std::size_t source = x * 3U;
            const std::size_t destination = (y * width + x) * 3U;
            image.rgb[destination] = row[source + 2U];
            image.rgb[destination + 1U] = row[source + 1U];
            image.rgb[destination + 2U] = row[source];
        }
    }
    return image;
}

void require_matching_dimensions(const Image& reference, const Image& actual) {
    if (reference.width != actual.width || reference.height != actual.height ||
        reference.rgb.size() != actual.rgb.size())
        throw std::invalid_argument("visual comparison image dimensions differ");
}

}  // namespace

Image Image::read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("failed to open image: " + path.string());
    const std::string extension = path.extension().string();
    if (extension == ".ppm" || extension == ".PPM") return read_ppm(input);
    if (extension == ".bmp" || extension == ".BMP") return read_bmp(input);
    throw std::invalid_argument("visual regression supports .bmp and .ppm images");
}

void Image::write_ppm(const std::filesystem::path& path) const {
    if (width == 0 || height == 0 || rgb.size() != width * height * 3U)
        throw std::invalid_argument("invalid visual image dimensions");
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("failed to write image: " + path.string());
    output << "P6\n" << width << ' ' << height << "\n255\n";
    output.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
}

Metrics compare(const Image& reference, const Image& actual) {
    require_matching_dimensions(reference, actual);
    double absolute_sum = 0.0;
    double squared_sum = 0.0;
    unsigned int maximum = 0;
    std::size_t changed = 0;
    for (std::size_t index = 0; index < reference.rgb.size(); ++index) {
        const unsigned int error = static_cast<unsigned int>(std::abs(
            static_cast<int>(reference.rgb[index]) - static_cast<int>(actual.rgb[index])));
        absolute_sum += error;
        squared_sum += static_cast<double>(error * error);
        maximum = std::max(maximum, error);
        if (error != 0U) ++changed;
    }
    const double channels = static_cast<double>(reference.rgb.size());
    return {absolute_sum / channels, std::sqrt(squared_sum / channels),
            static_cast<std::uint8_t>(maximum), changed};
}

Image difference_heatmap(const Image& reference, const Image& actual, unsigned int amplification) {
    require_matching_dimensions(reference, actual);
    Image difference{reference.width, reference.height, std::vector<std::uint8_t>(reference.rgb.size())};
    for (std::size_t index = 0; index < reference.rgb.size(); ++index) {
        const unsigned int error = static_cast<unsigned int>(std::abs(
            static_cast<int>(reference.rgb[index]) - static_cast<int>(actual.rgb[index])));
        difference.rgb[index] = static_cast<std::uint8_t>(std::min(255U, error * amplification));
    }
    return difference;
}

}  // namespace cg::visual
