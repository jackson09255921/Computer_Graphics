#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

#include "core/draw.hpp"
#include "core/image.hpp"
#include "curves/bezier.hpp"

int main(int argc, char** argv) {
    try {
        const std::filesystem::path output = argc > 1 ? argv[1] : "bezier_demo.bmp";
        cg::Image image(960, 540, {0.015, 0.022, 0.04});
        const cg::BezierCurve curve({{80.0, 440.0}, {250.0, 40.0}, {650.0, 500.0}, {880.0, 100.0}});
        const auto& controls = curve.control_points();

        for (std::size_t index = 1; index < controls.size(); ++index) {
            cg::draw_line(image, controls[index - 1], controls[index], {0.12, 0.18, 0.28});
        }
        for (const cg::Vec2 point : controls) {
            cg::draw_disc(image, point, 7.0, {1.0, 0.35, 0.12});
        }

        const std::vector<cg::Vec2> samples = curve.sample(400);
        for (std::size_t index = 1; index < samples.size(); ++index) {
            cg::draw_line(image, samples[index - 1], samples[index], {0.0, 0.8, 1.0});
        }

        const cg::Vec2 midpoint = curve.evaluate(0.5);
        const cg::Vec2 tangent = cg::normalized(curve.derivative(0.5));
        cg::draw_line(image, midpoint - tangent * 55.0, midpoint + tangent * 55.0, {0.4, 1.0, 0.35});
        cg::draw_disc(image, midpoint, 5.0, {1.0, 1.0, 1.0});

        image.write(output);
        std::cout << "wrote " << output.string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "bezier demo failed: " << error.what() << '\n';
        return 1;
    }
}
