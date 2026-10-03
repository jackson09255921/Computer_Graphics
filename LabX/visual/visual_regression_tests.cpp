#include "visual/visual_regression.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "core/image.hpp"

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    try {
        const std::filesystem::path directory = "visual-regression-test-output";
        std::filesystem::create_directories(directory);
        cg::Image source(3, 2);
        source.set(0, 0, {1.0, 0.0, 0.0});
        source.set(1, 0, {0.0, 1.0, 0.0});
        source.set(2, 0, {0.0, 0.0, 1.0});
        source.set(0, 1, {0.25, 0.5, 0.75});
        source.set(1, 1, {1.0, 1.0, 1.0});
        source.set(2, 1, {0.0, 0.0, 0.0});
        const auto bmp_path = directory / "roundtrip.bmp";
        const auto ppm_path = directory / "roundtrip.ppm";
        source.write_bmp(bmp_path);
        source.write_ppm(ppm_path);
        const cg::visual::Image bmp = cg::visual::Image::read(bmp_path);
        const cg::visual::Image ppm = cg::visual::Image::read(ppm_path);
        const cg::visual::Metrics identical = cg::visual::compare(bmp, ppm);
        require(bmp.width == 3 && bmp.height == 2, "BMP dimensions were not preserved");
        require(identical.mae == 0.0 && identical.rmse == 0.0 && identical.changed_channels == 0,
                "BMP and PPM round trips should match exactly");

        cg::visual::Image changed = ppm;
        changed.rgb[0] = static_cast<std::uint8_t>(changed.rgb[0] - 10U);
        const cg::visual::Metrics difference = cg::visual::compare(ppm, changed);
        require(std::abs(difference.mae - 10.0 / 18.0) < 1.0e-9, "MAE calculation is incorrect");
        require(difference.maximum_error == 10 && difference.changed_channels == 1,
                "visual difference statistics are incorrect");
        const auto diff_path = directory / "difference.ppm";
        cg::visual::difference_heatmap(ppm, changed).write_ppm(diff_path);
        require(cg::visual::Image::read(diff_path).rgb[0] == 40U, "heatmap amplification is incorrect");
        std::filesystem::remove_all(directory);
        std::cout << "visual regression tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "visual regression tests failed: " << error.what() << '\n';
        return 1;
    }
}
