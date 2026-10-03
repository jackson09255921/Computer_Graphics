#include "assets/asc_loader.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cg::assets {

AscMesh AscMesh::load(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("failed to open ASC mesh: " + path.string());
    std::size_t vertex_count = 0;
    std::size_t face_count = 0;
    if (!(input >> vertex_count >> face_count) || vertex_count == 0 || face_count == 0)
        throw std::runtime_error("invalid ASC mesh header: " + path.string());
    AscMesh mesh;
    mesh.vertices.resize(vertex_count);
    for (Vec3& vertex : mesh.vertices) {
        if (!(input >> vertex.x >> vertex.y >> vertex.z))
            throw std::runtime_error("truncated ASC vertex data: " + path.string());
    }
    for (std::size_t face = 0; face < face_count; ++face) {
        std::size_t corners = 0;
        if (!(input >> corners) || corners < 3)
            throw std::runtime_error("invalid ASC face: " + path.string());
        std::vector<std::size_t> indices(corners);
        for (std::size_t& index : indices) {
            if (!(input >> index) || index == 0 || index > vertex_count)
                throw std::runtime_error("ASC face index is out of range: " + path.string());
            --index;
        }
        for (std::size_t corner = 1; corner + 1 < corners; ++corner)
            mesh.triangles.push_back({indices[0], indices[corner], indices[corner + 1]});
    }
    if (mesh.triangles.empty()) throw std::runtime_error("ASC mesh has no triangles: " + path.string());
    return mesh;
}

}  // namespace cg::assets
