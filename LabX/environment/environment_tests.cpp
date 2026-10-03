#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "environment/environment_map.hpp"

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    const std::filesystem::path path = "environment_test.hdr";
    try {
        {
            std::ofstream output(path, std::ios::binary);
            output << "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 2 +X 2\n";
            const std::array<unsigned char, 16> pixels{
                128, 64, 32, 129, 128, 64, 32, 129,
                128, 64, 32, 129, 128, 64, 32, 129};
            output.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
        }
        const cg::environment::EnvironmentMap map = cg::environment::EnvironmentMap::load_radiance(path, 2.0);
        require(map.width() == 2 && map.height() == 2, "HDR dimensions must be preserved");
        const cg::Color color = map.sample({1.0, 0.0, 0.0});
        require(std::abs(color.x - 2.0) < 1e-9 && std::abs(color.y - 1.0) < 1e-9 &&
                    std::abs(color.z - 0.5) < 1e-9,
                "RGBE values and environment intensity must remain high dynamic range");

        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 1 +X 8\n";
            const std::array<unsigned char, 12> scanline{
                2, 2, 0, 8, 136, 128, 136, 64, 136, 32, 136, 129};
            output.write(reinterpret_cast<const char*>(scanline.data()), scanline.size());
        }
        const cg::environment::EnvironmentMap rle_map = cg::environment::EnvironmentMap::load_radiance(path);
        const cg::Color rle_color = rle_map.sample({0.0, 0.0, 1.0});
        require(rle_map.width() == 8 && std::abs(rle_color.x - 1.0) < 1e-9,
                "Radiance scanline RLE must decode channel runs");
        std::filesystem::remove(path);
        std::cout << "environment tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << "environment tests failed: " << error.what() << '\n';
        return 1;
    }
}
