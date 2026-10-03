#pragma once

#include <filesystem>
#include <vector>

#include "core/math.hpp"

namespace cg::assets {

struct AscTriangle {
    std::size_t first{};
    std::size_t second{};
    std::size_t third{};
};

struct AscMesh {
    std::vector<Vec3> vertices;
    std::vector<AscTriangle> triangles;

    static AscMesh load(const std::filesystem::path& path);
};

}  // namespace cg::assets
