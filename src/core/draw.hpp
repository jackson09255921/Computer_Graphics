#pragma once

#include <cstddef>

#include "core/image.hpp"
#include "core/math.hpp"

namespace cg {

void draw_line(Image& image, Vec2 start, Vec2 end, Color color);
void draw_disc(Image& image, Vec2 center, double radius, Color color);

}  // namespace cg
