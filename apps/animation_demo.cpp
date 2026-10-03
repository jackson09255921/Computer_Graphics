#include <array>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "animation/animation.hpp"
#include "core/draw.hpp"
#include "core/image.hpp"

namespace {

constexpr std::size_t kWidth = 640;
constexpr std::size_t kHeight = 360;
constexpr double kPi = 3.14159265358979323846;

const std::array<cg::Vec3, 8> kCubeVertices{{
    {-1.0, -1.0, -1.0}, {1.0, -1.0, -1.0}, {1.0, 1.0, -1.0}, {-1.0, 1.0, -1.0},
    {-1.0, -1.0, 1.0},  {1.0, -1.0, 1.0},  {1.0, 1.0, 1.0},  {-1.0, 1.0, 1.0},
}};
const std::array<std::array<std::size_t, 2>, 12> kCubeEdges{{
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
}};

cg::Vec2 project(const cg::Vec3& point, std::size_t width = kWidth, std::size_t height = kHeight) {
    const double camera_distance = 7.5;
    const double depth = std::max(0.5, camera_distance - point.z);
    const double focal_length = 360.0;
    return {static_cast<double>(width) * 0.5 + point.x * focal_length / depth,
            static_cast<double>(height) * 0.55 - point.y * focal_length / depth};
}

void render_cube(cg::Image& image, const cg::animation::Transform& transform, cg::Vec2 offset = {}) {
    std::array<cg::Vec2, 8> projected{};
    for (std::size_t index = 0; index < kCubeVertices.size(); ++index) {
        projected[index] = project(cg::animation::transform_point(transform, kCubeVertices[index])) + offset;
    }
    for (const auto& edge : kCubeEdges) {
        cg::draw_line(image, projected[edge[0]], projected[edge[1]], {0.0, 0.82, 1.0});
    }
    cg::draw_disc(image, project(transform.position) + offset, 4.0, {1.0, 0.35, 0.12});
}

cg::animation::AnimationClip make_clip() {
    using cg::animation::Keyframe;
    return cg::animation::AnimationClip({
        Keyframe{0.0, {{-1.8, -0.5, -1.0}, cg::from_axis_angle({0.0, 1.0, 0.0}, 0.0), {0.65, 0.65, 0.65}}},
        Keyframe{1.0, {{0.0, 1.0, 0.0}, cg::from_axis_angle({1.0, 1.0, 0.0}, kPi), {1.0, 1.0, 1.0}}},
        Keyframe{2.0, {{1.8, -0.5, -1.0}, cg::from_axis_angle({0.0, 1.0, 0.0}, 1.8 * kPi), {0.65, 0.65, 0.65}}},
    });
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const std::filesystem::path output_directory = argc > 1 ? argv[1] : "animation_frames";
        std::filesystem::create_directories(output_directory);
        const cg::animation::AnimationClip clip = make_clip();
        constexpr std::size_t frame_count = 60;

        for (std::size_t frame = 0; frame < frame_count; ++frame) {
            cg::Image image(kWidth, kHeight, {0.015, 0.022, 0.04});
            cg::draw_line(image, {40.0, 300.0}, {600.0, 300.0}, {0.1, 0.15, 0.24});
            const double time = clip.duration() * static_cast<double>(frame) / static_cast<double>(frame_count - 1);
            render_cube(image, clip.sample(time));
            std::ostringstream name;
            name << "frame_" << std::setw(3) << std::setfill('0') << frame << ".bmp";
            image.write_bmp(output_directory / name.str());
        }

        cg::Image contact_sheet(960, 540, {0.015, 0.022, 0.04});
        for (std::size_t index = 0; index < 9; ++index) {
            const double time = clip.duration() * static_cast<double>(index) / 8.0;
            const cg::Vec2 offset{static_cast<double>((index % 3) * 320) - 160.0,
                                  static_cast<double>((index / 3) * 180) - 108.0};
            render_cube(contact_sheet, clip.sample(time), offset);
        }
        contact_sheet.write_bmp(output_directory / "contact_sheet.bmp");
        std::cout << "wrote " << frame_count << " frames and contact sheet to " << output_directory.string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "animation demo failed: " << error.what() << '\n';
        return 1;
    }
}
