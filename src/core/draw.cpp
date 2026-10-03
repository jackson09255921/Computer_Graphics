#include "core/draw.hpp"

#include <algorithm>
#include <cmath>

namespace cg {
namespace {

void set_if_visible(Image& image, int x, int y, Color color) {
    if (x >= 0 && y >= 0 && static_cast<std::size_t>(x) < image.width() &&
        static_cast<std::size_t>(y) < image.height()) {
        image.set(static_cast<std::size_t>(x), static_cast<std::size_t>(y), color);
    }
}

}  // namespace

void draw_line(Image& image, Vec2 start, Vec2 end, Color color) {
    int x0 = static_cast<int>(std::lround(start.x));
    int y0 = static_cast<int>(std::lround(start.y));
    const int x1 = static_cast<int>(std::lround(end.x));
    const int y1 = static_cast<int>(std::lround(end.y));
    const int dx = std::abs(x1 - x0);
    const int step_x = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int step_y = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    while (true) {
        set_if_visible(image, x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int doubled = 2 * error;
        if (doubled >= dy) {
            error += dy;
            x0 += step_x;
        }
        if (doubled <= dx) {
            error += dx;
            y0 += step_y;
        }
    }
}

void draw_disc(Image& image, Vec2 center, double radius, Color color) {
    const int min_x = static_cast<int>(std::floor(center.x - radius));
    const int max_x = static_cast<int>(std::ceil(center.x + radius));
    const int min_y = static_cast<int>(std::floor(center.y - radius));
    const int max_y = static_cast<int>(std::ceil(center.y + radius));
    const double radius_squared = radius * radius;
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            const double dx = static_cast<double>(x) - center.x;
            const double dy = static_cast<double>(y) - center.y;
            if (dx * dx + dy * dy <= radius_squared) {
                set_if_visible(image, x, y, color);
            }
        }
    }
}

}  // namespace cg
