#include "assets/gltf_loader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "stb_image.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace cg::assets {
namespace {

struct Json {
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json>;
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;

    [[nodiscard]] const Object& object() const { return std::get<Object>(value); }
    [[nodiscard]] const Array& array() const { return std::get<Array>(value); }
    [[nodiscard]] const std::string& string() const { return std::get<std::string>(value); }
    [[nodiscard]] double number() const { return std::get<double>(value); }
    [[nodiscard]] const Json* find(const std::string& key) const {
        const auto& values = object();
        const auto found = values.find(key);
        return found == values.end() ? nullptr : &found->second;
    }
};

class JsonParser {
public:
    explicit JsonParser(std::string source) : source_(std::move(source)) {}
    Json parse() {
        Json result = value();
        whitespace();
        if (position_ != source_.size()) fail("trailing JSON content");
        return result;
    }

private:
    [[noreturn]] void fail(const char* message) const {
        throw std::runtime_error(std::string("invalid glTF JSON at byte ") + std::to_string(position_) + ": " + message);
    }
    void whitespace() {
        while (position_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[position_]))) ++position_;
    }
    bool consume(char expected) {
        whitespace();
        if (position_ < source_.size() && source_[position_] == expected) { ++position_; return true; }
        return false;
    }
    Json value() {
        whitespace();
        if (position_ >= source_.size()) fail("expected value");
        const char token = source_[position_];
        if (token == '{') return Json{object()};
        if (token == '[') return Json{array()};
        if (token == '"') return Json{string()};
        if (token == '-' || std::isdigit(static_cast<unsigned char>(token))) return Json{number()};
        if (source_.compare(position_, 4, "true") == 0) { position_ += 4; return Json{true}; }
        if (source_.compare(position_, 5, "false") == 0) { position_ += 5; return Json{false}; }
        if (source_.compare(position_, 4, "null") == 0) { position_ += 4; return Json{nullptr}; }
        fail("unknown value");
    }
    Json::Object object() {
        consume('{');
        Json::Object result;
        if (consume('}')) return result;
        do {
            whitespace();
            if (position_ >= source_.size() || source_[position_] != '"') fail("expected object key");
            std::string key = string();
            if (!consume(':')) fail("expected colon");
            result.emplace(std::move(key), value());
        } while (consume(','));
        if (!consume('}')) fail("expected closing brace");
        return result;
    }
    Json::Array array() {
        consume('[');
        Json::Array result;
        if (consume(']')) return result;
        do { result.push_back(value()); } while (consume(','));
        if (!consume(']')) fail("expected closing bracket");
        return result;
    }
    std::string string() {
        if (!consume('"')) fail("expected string");
        std::string result;
        while (position_ < source_.size()) {
            char character = source_[position_++];
            if (character == '"') return result;
            if (character == '\\') {
                if (position_ >= source_.size()) fail("unfinished escape");
                character = source_[position_++];
                if (character == 'n') character = '\n';
                else if (character == 'r') character = '\r';
                else if (character == 't') character = '\t';
                else if (character == 'u') fail("unicode escapes are not supported in asset URIs");
            }
            result.push_back(character);
        }
        fail("unterminated string");
    }
    double number() {
        const char* begin = source_.c_str() + position_;
        char* end = nullptr;
        const double result = std::strtod(begin, &end);
        if (end == begin) fail("invalid number");
        position_ = static_cast<std::size_t>(end - source_.c_str());
        return result;
    }

    std::string source_;
    std::size_t position_{0};
};

std::size_t integer(const Json& value) {
    const double number = value.number();
    if (number < 0.0 || number != static_cast<double>(static_cast<std::size_t>(number))) {
        throw std::runtime_error("glTF index must be a non-negative integer");
    }
    return static_cast<std::size_t>(number);
}

std::size_t member_integer(const Json& object, const std::string& key, std::size_t fallback = 0) {
    const Json* value = object.find(key);
    return value ? integer(*value) : fallback;
}

std::vector<std::uint8_t> read_binary(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("failed to open glTF buffer: " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::uint32_t little_u32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 4 > bytes.size()) throw std::runtime_error("truncated GLB header or chunk");
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
}

struct Document {
    std::string json;
    std::vector<std::uint8_t> binary;
    bool embedded_binary{false};
};

Document read_document(const std::filesystem::path& path) {
    const std::vector<std::uint8_t> bytes = read_binary(path);
    if (bytes.size() < 4 || std::memcmp(bytes.data(), "glTF", 4) != 0)
        return {std::string(bytes.begin(), bytes.end()), {}, false};
    if (bytes.size() < 20 || little_u32(bytes, 4) != 2)
        throw std::runtime_error("only GLB version 2 is supported");
    if (little_u32(bytes, 8) != bytes.size())
        throw std::runtime_error("GLB declared length does not match file size");
    std::size_t cursor = 12;
    std::string json;
    std::vector<std::uint8_t> binary;
    while (cursor < bytes.size()) {
        const std::uint32_t length = little_u32(bytes, cursor);
        const std::uint32_t type = little_u32(bytes, cursor + 4);
        cursor += 8;
        if (cursor + length > bytes.size()) throw std::runtime_error("GLB chunk exceeds file bounds");
        if (type == 0x4e4f534au) {
            if (!json.empty()) throw std::runtime_error("GLB contains multiple JSON chunks");
            json.assign(reinterpret_cast<const char*>(bytes.data() + cursor), length);
            while (!json.empty() && (json.back() == ' ' || json.back() == '\0')) json.pop_back();
        } else if (type == 0x004e4942u) {
            if (!binary.empty()) throw std::runtime_error("GLB contains multiple BIN chunks");
            binary.assign(bytes.begin() + static_cast<std::ptrdiff_t>(cursor),
                          bytes.begin() + static_cast<std::ptrdiff_t>(cursor + length));
        }
        cursor += length;
    }
    if (json.empty()) throw std::runtime_error("GLB has no JSON chunk");
    return {std::move(json), std::move(binary), true};
}

struct Matrix {
    std::array<double, 16> m{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

Matrix multiply(const Matrix& lhs, const Matrix& rhs) {
    Matrix result{{}};
    for (std::size_t column = 0; column < 4; ++column)
        for (std::size_t row = 0; row < 4; ++row)
            for (std::size_t k = 0; k < 4; ++k)
                result.m[column * 4 + row] += lhs.m[k * 4 + row] * rhs.m[column * 4 + k];
    return result;
}

Vec3 transform(const Matrix& matrix, const Vec3& value) {
    return {matrix.m[0] * value.x + matrix.m[4] * value.y + matrix.m[8] * value.z + matrix.m[12],
            matrix.m[1] * value.x + matrix.m[5] * value.y + matrix.m[9] * value.z + matrix.m[13],
            matrix.m[2] * value.x + matrix.m[6] * value.y + matrix.m[10] * value.z + matrix.m[14]};
}

Vec3 transform_normal(const Matrix& matrix, const Vec3& value) {
    const double a00 = matrix.m[0], a01 = matrix.m[4], a02 = matrix.m[8];
    const double a10 = matrix.m[1], a11 = matrix.m[5], a12 = matrix.m[9];
    const double a20 = matrix.m[2], a21 = matrix.m[6], a22 = matrix.m[10];
    const double c00 = a11*a22 - a12*a21, c01 = a12*a20 - a10*a22, c02 = a10*a21 - a11*a20;
    const double c10 = a02*a21 - a01*a22, c11 = a00*a22 - a02*a20, c12 = a01*a20 - a00*a21;
    const double c20 = a01*a12 - a02*a11, c21 = a02*a10 - a00*a12, c22 = a00*a11 - a01*a10;
    const double determinant = a00*c00 + a01*c01 + a02*c02;
    if (std::abs(determinant) <= kEpsilon) return {};
    return normalized(Vec3{c00*value.x + c01*value.y + c02*value.z,
                           c10*value.x + c11*value.y + c12*value.z,
                           c20*value.x + c21*value.y + c22*value.z} / determinant);
}

Matrix node_matrix(const Json& node) {
    if (const Json* matrix = node.find("matrix")) {
        if (matrix->array().size() != 16) throw std::runtime_error("glTF node matrix must contain 16 values");
        Matrix result;
        for (std::size_t i = 0; i < 16; ++i) result.m[i] = matrix->array()[i].number();
        return result;
    }
    Vec3 translation{};
    Vec3 scale{1, 1, 1};
    std::array<double, 4> rotation{0, 0, 0, 1};
    if (const Json* values = node.find("translation"))
        translation = {values->array()[0].number(), values->array()[1].number(), values->array()[2].number()};
    if (const Json* values = node.find("scale"))
        scale = {values->array()[0].number(), values->array()[1].number(), values->array()[2].number()};
    if (const Json* values = node.find("rotation"))
        for (std::size_t i = 0; i < 4; ++i) rotation[i] = values->array()[i].number();
    const double x = rotation[0], y = rotation[1], z = rotation[2], w = rotation[3];
    Matrix result;
    result.m = {
        (1 - 2*y*y - 2*z*z) * scale.x, (2*x*y + 2*z*w) * scale.x, (2*x*z - 2*y*w) * scale.x, 0,
        (2*x*y - 2*z*w) * scale.y, (1 - 2*x*x - 2*z*z) * scale.y, (2*y*z + 2*x*w) * scale.y, 0,
        (2*x*z + 2*y*w) * scale.z, (2*y*z - 2*x*w) * scale.z, (1 - 2*x*x - 2*y*y) * scale.z, 0,
        translation.x, translation.y, translation.z, 1};
    return result;
}

struct View { std::size_t offset{}, length{}, stride{}; };
struct Accessor { std::size_t view{}, offset{}, count{}, component{}; std::string type; };

template <typename T>
T scalar_at(const std::vector<std::uint8_t>& buffer, std::size_t offset) {
    if (offset + sizeof(T) > buffer.size()) throw std::runtime_error("glTF accessor exceeds buffer bounds");
    T value{};
    std::memcpy(&value, buffer.data() + offset, sizeof(T));
    return value;
}

std::vector<Vec3> positions(const Accessor& accessor, const View& view,
                            const std::vector<std::uint8_t>& buffer) {
    if (accessor.component != 5126 || accessor.type != "VEC3")
        throw std::runtime_error("POSITION accessor must use FLOAT VEC3");
    const std::size_t stride = view.stride ? view.stride : sizeof(float) * 3;
    std::vector<Vec3> result(accessor.count);
    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t offset = view.offset + accessor.offset + i * stride;
        result[i] = {scalar_at<float>(buffer, offset), scalar_at<float>(buffer, offset + 4),
                     scalar_at<float>(buffer, offset + 8)};
    }
    return result;
}

std::vector<Vec2> texcoords(const Accessor& accessor, const View& view,
                            const std::vector<std::uint8_t>& buffer) {
    if (accessor.component != 5126 || accessor.type != "VEC2")
        throw std::runtime_error("TEXCOORD_0 accessor must use FLOAT VEC2");
    const std::size_t stride = view.stride ? view.stride : sizeof(float) * 2;
    std::vector<Vec2> result(accessor.count);
    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t offset = view.offset + accessor.offset + i * stride;
        result[i] = {scalar_at<float>(buffer, offset), scalar_at<float>(buffer, offset + 4)};
    }
    return result;
}

std::vector<std::uint32_t> indices(const Accessor& accessor, const View& view,
                                   const std::vector<std::uint8_t>& buffer) {
    std::size_t size = 0;
    if (accessor.component == 5121) size = 1;
    else if (accessor.component == 5123) size = 2;
    else if (accessor.component == 5125) size = 4;
    else throw std::runtime_error("indices must use UNSIGNED_BYTE, UNSIGNED_SHORT, or UNSIGNED_INT");
    const std::size_t stride = view.stride ? view.stride : size;
    std::vector<std::uint32_t> result(accessor.count);
    for (std::size_t i = 0; i < accessor.count; ++i) {
        const std::size_t offset = view.offset + accessor.offset + i * stride;
        result[i] = size == 1 ? scalar_at<std::uint8_t>(buffer, offset) :
                    size == 2 ? scalar_at<std::uint16_t>(buffer, offset) : scalar_at<std::uint32_t>(buffer, offset);
    }
    return result;
}

rt::Material material_at(const Json& root, std::size_t index) {
    rt::Material result;
    const Json* materials = root.find("materials");
    if (!materials || index >= materials->array().size()) return result;
    const Json* pbr = materials->array()[index].find("pbrMetallicRoughness");
    if (!pbr) return result;
    if (const Json* color = pbr->find("baseColorFactor"))
        result.albedo = {color->array()[0].number(), color->array()[1].number(), color->array()[2].number()};
    if (const Json* metallic = pbr->find("metallicFactor")) result.metallic = metallic->number();
    if (const Json* roughness = pbr->find("roughnessFactor")) result.roughness = roughness->number();
    return result;
}

}  // namespace

Color GltfTexture::sample(Vec2 uv) const {
    if (pixels.empty() || width == 0 || height == 0) return {1.0, 1.0, 1.0};
    uv.x -= std::floor(uv.x);
    uv.y -= std::floor(uv.y);
    const double x = uv.x * static_cast<double>(width) - 0.5;
    const double y = uv.y * static_cast<double>(height) - 0.5;
    const auto texel = [&](long long column, long long row) {
        const auto wrap = [](long long value, std::size_t size) {
            const long long length = static_cast<long long>(size);
            return static_cast<std::size_t>((value % length + length) % length);
        };
        return pixels[wrap(row, height) * width + wrap(column, width)];
    };
    const long long x0 = static_cast<long long>(std::floor(x));
    const long long y0 = static_cast<long long>(std::floor(y));
    const double tx = x - std::floor(x), ty = y - std::floor(y);
    return lerp(lerp(texel(x0, y0), texel(x0 + 1, y0), tx),
                lerp(texel(x0, y0 + 1), texel(x0 + 1, y0 + 1), tx), ty);
}

GltfAsset GltfAsset::load(const std::filesystem::path& path) {
    const Document document = read_document(path);
    const Json root = JsonParser(document.json).parse();
    const Json* asset = root.find("asset");
    if (!asset || !asset->find("version") || asset->find("version")->string().rfind("2.", 0) != 0)
        throw std::runtime_error("only glTF 2.x assets are supported");
    const auto& buffers = root.find("buffers")->array();
    if (buffers.size() != 1)
        throw std::runtime_error("this LabX loader requires exactly one buffer");
    std::vector<std::uint8_t> buffer;
    if (document.embedded_binary) {
        if (buffers[0].find("uri")) throw std::runtime_error("GLB buffer must not declare an external URI");
        buffer = document.binary;
    } else {
        const Json* uri_value = buffers[0].find("uri");
        if (!uri_value) throw std::runtime_error("JSON glTF buffer requires an external URI");
        const std::string uri = uri_value->string();
        if (uri.find("..") != std::string::npos || uri.find(':') != std::string::npos)
            throw std::runtime_error("unsafe or embedded glTF buffer URI");
        buffer = read_binary(path.parent_path() / uri);
    }
    if (const Json* length = buffers[0].find("byteLength"))
        if (buffer.size() < integer(*length)) throw std::runtime_error("glTF buffer is shorter than byteLength");

    std::vector<View> views;
    for (const Json& source : root.find("bufferViews")->array())
        views.push_back({member_integer(source, "byteOffset"), member_integer(source, "byteLength"),
                         member_integer(source, "byteStride")});
    std::vector<Accessor> accessors;
    for (const Json& source : root.find("accessors")->array())
        accessors.push_back({member_integer(source, "bufferView"), member_integer(source, "byteOffset"),
                             member_integer(source, "count"), member_integer(source, "componentType"),
                             source.find("type")->string()});

    std::vector<std::shared_ptr<const GltfTexture>> textures;
    if (const Json* texture_sources = root.find("textures")) {
        const Json* images = root.find("images");
        if (!images) throw std::runtime_error("glTF textures require images");
        for (const Json& texture_source : texture_sources->array()) {
            const Json& image = images->array().at(member_integer(texture_source, "source"));
            const Json* image_view = image.find("bufferView");
            if (!image_view) throw std::runtime_error("only bufferView-backed glTF images are supported");
            const View& view = views.at(integer(*image_view));
            if (view.offset + view.length > buffer.size()) throw std::runtime_error("glTF image exceeds buffer bounds");
            int width = 0, height = 0, channels = 0;
            stbi_uc* decoded = stbi_load_from_memory(buffer.data() + view.offset, static_cast<int>(view.length),
                                                     &width, &height, &channels, 4);
            if (!decoded) throw std::runtime_error(std::string("failed to decode glTF image: ") + stbi_failure_reason());
            auto texture = std::make_shared<GltfTexture>();
            texture->width = static_cast<std::size_t>(width);
            texture->height = static_cast<std::size_t>(height);
            texture->pixels.reserve(texture->width * texture->height);
            for (std::size_t pixel = 0; pixel < texture->width * texture->height; ++pixel)
                texture->pixels.push_back({decoded[pixel * 4] / 255.0,
                                           decoded[pixel * 4 + 1] / 255.0,
                                           decoded[pixel * 4 + 2] / 255.0});
            stbi_image_free(decoded);
            textures.push_back(std::move(texture));
        }
    }

    GltfAsset result;
    const auto& meshes = root.find("meshes")->array();
    const auto emit_mesh = [&](std::size_t mesh_index, const Matrix& world) {
        for (const Json& primitive : meshes.at(mesh_index).find("primitives")->array()) {
            if (member_integer(primitive, "mode", 4) != 4) throw std::runtime_error("only TRIANGLES mode is supported");
            const Json* attributes = primitive.find("attributes");
            const Json* position = attributes ? attributes->find("POSITION") : nullptr;
            if (!position) throw std::runtime_error("glTF triangle primitive has no POSITION attribute");
            const std::size_t position_index = integer(*position);
            const Accessor& position_accessor = accessors.at(position_index);
            std::vector<Vec3> vertices = positions(position_accessor, views.at(position_accessor.view), buffer);
            std::vector<Vec3> vertex_normals;
            if (const Json* normal = attributes->find("NORMAL")) {
                const Accessor& normal_accessor = accessors.at(integer(*normal));
                vertex_normals = positions(normal_accessor, views.at(normal_accessor.view), buffer);
                if (vertex_normals.size() != vertices.size())
                    throw std::runtime_error("NORMAL and POSITION accessor counts must match");
            }
            std::vector<Vec2> vertex_uvs;
            if (const Json* uv = attributes->find("TEXCOORD_0")) {
                const Accessor& uv_accessor = accessors.at(integer(*uv));
                vertex_uvs = texcoords(uv_accessor, views.at(uv_accessor.view), buffer);
                if (vertex_uvs.size() != vertices.size())
                    throw std::runtime_error("TEXCOORD_0 and POSITION accessor counts must match");
            }
            std::vector<std::uint32_t> element_indices;
            if (const Json* index = primitive.find("indices")) {
                const Accessor& index_accessor = accessors.at(integer(*index));
                element_indices = indices(index_accessor, views.at(index_accessor.view), buffer);
            } else {
                for (std::size_t i = 0; i < vertices.size(); ++i) element_indices.push_back(static_cast<std::uint32_t>(i));
            }
            if (element_indices.size() % 3 != 0) throw std::runtime_error("triangle index count must be divisible by three");
            rt::Material material;
            std::shared_ptr<const GltfTexture> base_color_texture;
            std::shared_ptr<const GltfTexture> normal_texture;
            std::shared_ptr<const GltfTexture> metallic_roughness_texture;
            std::shared_ptr<const GltfTexture> emissive_texture;
            Color emissive_factor{};
            double normal_scale = 1.0;
            if (const Json* material_index = primitive.find("material")) {
                const std::size_t index = integer(*material_index);
                material = material_at(root, index);
                const Json& material_source = root.find("materials")->array().at(index);
                const Json* pbr = material_source.find("pbrMetallicRoughness");
                if (pbr) {
                    if (const Json* texture = pbr->find("baseColorTexture"))
                        base_color_texture = textures.at(member_integer(*texture, "index"));
                    if (const Json* texture = pbr->find("metallicRoughnessTexture"))
                        metallic_roughness_texture = textures.at(member_integer(*texture, "index"));
                }
                if (const Json* texture = material_source.find("normalTexture")) {
                    normal_texture = textures.at(member_integer(*texture, "index"));
                    if (const Json* scale = texture->find("scale")) normal_scale = scale->number();
                }
                if (const Json* texture = material_source.find("emissiveTexture"))
                    emissive_texture = textures.at(member_integer(*texture, "index"));
                if (const Json* factor = material_source.find("emissiveFactor"))
                    emissive_factor = {factor->array()[0].number(), factor->array()[1].number(),
                                       factor->array()[2].number()};
            }
            for (std::size_t i = 0; i < element_indices.size(); i += 3) {
                const Vec3 a = transform(world, vertices.at(element_indices[i]));
                const Vec3 b = transform(world, vertices.at(element_indices[i + 1]));
                const Vec3 c = transform(world, vertices.at(element_indices[i + 2]));
                const Vec3 face_normal = normalized(cross(b - a, c - a));
                if (length(face_normal) <= kEpsilon) continue;
                if (vertex_normals.empty()) {
                    result.triangles_.push_back({a, b, c, {}, {}, {}, false,
                        vertex_uvs.empty() ? Vec2{} : vertex_uvs.at(element_indices[i]),
                        vertex_uvs.empty() ? Vec2{} : vertex_uvs.at(element_indices[i + 1]),
                        vertex_uvs.empty() ? Vec2{} : vertex_uvs.at(element_indices[i + 2]),
                        base_color_texture, normal_texture, metallic_roughness_texture,
                        emissive_texture, emissive_factor, normal_scale, material});
                } else {
                    const auto normal_or_face = [&](std::uint32_t vertex) {
                        const Vec3 transformed = transform_normal(world, vertex_normals.at(vertex));
                        return length(transformed) <= kEpsilon ? face_normal : transformed;
                    };
                    result.triangles_.push_back({a, b, c,
                        normal_or_face(element_indices[i]), normal_or_face(element_indices[i + 1]),
                        normal_or_face(element_indices[i + 2]), true,
                        vertex_uvs.empty() ? Vec2{} : vertex_uvs.at(element_indices[i]),
                        vertex_uvs.empty() ? Vec2{} : vertex_uvs.at(element_indices[i + 1]),
                        vertex_uvs.empty() ? Vec2{} : vertex_uvs.at(element_indices[i + 2]),
                        base_color_texture, normal_texture, metallic_roughness_texture,
                        emissive_texture, emissive_factor, normal_scale, material});
                }
            }
        }
    };

    const auto& nodes = root.find("nodes")->array();
    std::vector<bool> active(nodes.size(), false);
    const auto visit = [&](auto&& self, std::size_t node_index, const Matrix& parent) -> void {
        if (active.at(node_index)) throw std::runtime_error("glTF node hierarchy contains a cycle");
        active[node_index] = true;
        const Json& node = nodes.at(node_index);
        const Matrix world = multiply(parent, node_matrix(node));
        if (const Json* mesh = node.find("mesh")) emit_mesh(integer(*mesh), world);
        if (const Json* children = node.find("children"))
            for (const Json& child : children->array()) self(self, integer(child), world);
        active[node_index] = false;
    };
    if (const Json* scenes = root.find("scenes")) {
        const std::size_t scene_index = member_integer(root, "scene");
        const Json& scene = scenes->array().at(scene_index);
        if (const Json* scene_nodes = scene.find("nodes"))
            for (const Json& node : scene_nodes->array()) visit(visit, integer(node), Matrix{});
    } else {
        std::vector<bool> is_child(nodes.size(), false);
        for (const Json& node : nodes)
            if (const Json* children = node.find("children"))
                for (const Json& child : children->array()) is_child.at(integer(child)) = true;
        for (std::size_t index = 0; index < nodes.size(); ++index)
            if (!is_child[index]) visit(visit, index, Matrix{});
    }
    return result;
}

void GltfAsset::add_to(rt::Scene& scene) const {
    for (const GltfTriangle& triangle : triangles_) {
        if (triangle.has_normals)
            scene.add(std::make_shared<rt::Triangle>(triangle.first, triangle.second, triangle.third,
                triangle.first_normal, triangle.second_normal, triangle.third_normal, triangle.material));
        else
            scene.add(std::make_shared<rt::Triangle>(triangle.first, triangle.second, triangle.third,
                                                     triangle.material));
    }
}

}  // namespace cg::assets
