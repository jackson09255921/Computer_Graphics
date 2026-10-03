#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cuda_runtime.h>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stack_size.h>
#include <optix_stubs.h>

#include "optix_triangle_shared.h"
#include "assets/gltf_loader.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check_cuda(cudaError_t result, const char* operation) {
    if (result != cudaSuccess)
        throw std::runtime_error(std::string(operation) + ": " + cudaGetErrorString(result));
}

void check_optix(OptixResult result, const char* operation) {
    if (result != OPTIX_SUCCESS)
        throw std::runtime_error(std::string(operation) + ": " + optixGetErrorName(result));
}

template <typename T>
struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord {
    char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;
};

struct EmptyData {};
using EmptyRecord = SbtRecord<EmptyData>;

struct MeshData {
    std::vector<float3> vertices;
    std::vector<uint3> indices;
    std::vector<float3> colors;
    std::vector<float> reflectivity;
    std::vector<float> roughness;
};

MeshData default_mesh() {
    return {{{-1.0f, -0.8f, 0.0f}, {1.0f, -0.8f, 0.0f},
             {1.0f, 0.8f, 0.0f}, {-1.0f, 0.8f, 0.0f},
             {-0.30f, -0.25f, 0.45f}, {0.35f, -0.20f, 0.45f},
             {0.05f, 0.38f, 0.45f}},
            {{0, 1, 2}, {0, 2, 3}, {4, 5, 6}},
            {{0.72f, 0.72f, 0.72f}, {0.72f, 0.72f, 0.72f},
             {0.95f, 0.22f, 0.12f}},
            {0.08f, 0.08f, 0.72f},
            {0.55f, 0.55f, 0.12f}};
}

MeshData load_gltf_mesh(const std::string& path) {
    const cg::assets::GltfAsset asset = cg::assets::GltfAsset::load(path);
    if (asset.triangles().empty()) throw std::runtime_error("glTF scene contains no triangles");
    MeshData mesh;
    for (const auto& triangle : asset.triangles()) {
        const unsigned int first = static_cast<unsigned int>(mesh.vertices.size());
        mesh.vertices.push_back(make_float3(static_cast<float>(triangle.first.x),
                                            static_cast<float>(triangle.first.y),
                                            static_cast<float>(triangle.first.z)));
        mesh.vertices.push_back(make_float3(static_cast<float>(triangle.second.x),
                                            static_cast<float>(triangle.second.y),
                                            static_cast<float>(triangle.second.z)));
        mesh.vertices.push_back(make_float3(static_cast<float>(triangle.third.x),
                                            static_cast<float>(triangle.third.y),
                                            static_cast<float>(triangle.third.z)));
        mesh.indices.push_back(make_uint3(first, first + 1, first + 2));
        mesh.colors.push_back(make_float3(static_cast<float>(triangle.material.albedo.x),
                                          static_cast<float>(triangle.material.albedo.y),
                                          static_cast<float>(triangle.material.albedo.z)));
        const float metallic = static_cast<float>(triangle.material.metallic);
        const float roughness = static_cast<float>(triangle.material.roughness);
        const float fresnel = 0.04f * (1.0f - metallic) + metallic;
        mesh.reflectivity.push_back(std::clamp(fresnel, 0.0f, 0.95f));
        mesh.roughness.push_back(std::clamp(roughness, 0.02f, 1.0f));
    }
    return mesh;
}

std::string read_binary(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error(std::string("Cannot open PTX: ") + path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_ppm(const std::string& path, const std::vector<uchar4>& image,
               unsigned int width, unsigned int height) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("Cannot write image: " + path);
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (auto pixel : image) {
        output.put(static_cast<char>(pixel.x));
        output.put(static_cast<char>(pixel.y));
        output.put(static_cast<char>(pixel.z));
    }
}
}

int main(int argc, char** argv) {
    constexpr unsigned int width = 256;
    constexpr unsigned int height = 192;
    constexpr unsigned int samples_per_pixel = 32;
    const std::string output_path = argc > 1 ? argv[1] : "optix_triangle.ppm";
    MeshData mesh;

    OptixDeviceContext context = nullptr;
    OptixModule module = nullptr;
    OptixPipeline pipeline = nullptr;
    OptixProgramGroup raygen = nullptr, radiance_miss = nullptr, shadow_miss = nullptr;
    OptixProgramGroup radiance_hit = nullptr, shadow_hit = nullptr;
    CUdeviceptr gas = 0, raygen_record = 0, miss_record = 0, hit_record = 0;
    CUdeviceptr device_image = 0, device_accumulation = 0, device_params = 0;
    CUdeviceptr device_colors = 0, device_reflectivity = 0;
    CUdeviceptr device_roughness = 0;
    CUdeviceptr device_vertices = 0, device_indices = 0;

    try {
        mesh = argc > 2 ? load_gltf_mesh(argv[2]) : default_mesh();
        check_cuda(cudaFree(nullptr), "cudaFree initialization");
        check_optix(optixInit(), "optixInit");
        OptixDeviceContextOptions context_options{};
        check_optix(optixDeviceContextCreate(nullptr, &context_options, &context),
                    "optixDeviceContextCreate");

        const std::size_t vertices_size = mesh.vertices.size() * sizeof(float3);
        const std::size_t indices_size = mesh.indices.size() * sizeof(uint3);
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_vertices), vertices_size), "cudaMalloc vertices");
        check_cuda(cudaMemcpy(reinterpret_cast<void*>(device_vertices), mesh.vertices.data(), vertices_size,
                              cudaMemcpyHostToDevice), "cudaMemcpy vertices");
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_indices), indices_size), "cudaMalloc indices");
        check_cuda(cudaMemcpy(reinterpret_cast<void*>(device_indices), mesh.indices.data(), indices_size,
                              cudaMemcpyHostToDevice), "cudaMemcpy indices");
        const unsigned int geometry_flags[] = {OPTIX_GEOMETRY_FLAG_NONE};
        OptixBuildInput build_input{};
        build_input.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
        build_input.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
        build_input.triangleArray.numVertices = static_cast<unsigned int>(mesh.vertices.size());
        build_input.triangleArray.vertexBuffers = &device_vertices;
        build_input.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
        build_input.triangleArray.numIndexTriplets = static_cast<unsigned int>(mesh.indices.size());
        build_input.triangleArray.indexBuffer = device_indices;
        build_input.triangleArray.flags = geometry_flags;
        build_input.triangleArray.numSbtRecords = 1;
        OptixAccelBuildOptions accel_options{};
        accel_options.buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE;
        accel_options.operation = OPTIX_BUILD_OPERATION_BUILD;
        OptixAccelBufferSizes sizes{};
        check_optix(optixAccelComputeMemoryUsage(context, &accel_options, &build_input, 1, &sizes),
                    "optixAccelComputeMemoryUsage");
        CUdeviceptr scratch = 0;
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&scratch), sizes.tempSizeInBytes), "cudaMalloc GAS scratch");
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&gas), sizes.outputSizeInBytes), "cudaMalloc GAS");
        OptixTraversableHandle gas_handle{};
        check_optix(optixAccelBuild(context, nullptr, &accel_options, &build_input, 1,
                                    scratch, sizes.tempSizeInBytes, gas, sizes.outputSizeInBytes,
                                    &gas_handle, nullptr, 0), "optixAccelBuild");
        check_cuda(cudaFree(reinterpret_cast<void*>(scratch)), "cudaFree GAS scratch");

        OptixPipelineCompileOptions compile_options{};
        compile_options.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
        compile_options.numPayloadValues = 4;
        compile_options.numAttributeValues = 2;
        compile_options.pipelineLaunchParamsVariableName = "params";
        compile_options.pipelineLaunchParamsSizeInBytes = sizeof(OptixTriangleParams);
        compile_options.usesPrimitiveTypeFlags = OPTIX_PRIMITIVE_TYPE_FLAGS_TRIANGLE;
        OptixModuleCompileOptions module_options{};
        const std::string ptx = read_binary(OPTIX_TRIANGLE_PTX_PATH);
        std::array<char, 4096> log{};
        std::size_t log_size = log.size();
        const OptixResult module_result = optixModuleCreate(context, &module_options, &compile_options,
                                                            ptx.data(), ptx.size(), log.data(), &log_size, &module);
        if (module_result != OPTIX_SUCCESS)
            throw std::runtime_error(std::string("optixModuleCreate: ") + optixGetErrorName(module_result) +
                                     "\n" + log.data());

        OptixProgramGroupOptions group_options{};
        auto create_group = [&](OptixProgramGroupDesc& description, OptixProgramGroup& group, const char* name) {
            log.fill(0); log_size = log.size();
            const OptixResult result = optixProgramGroupCreate(context, &description, 1, &group_options,
                                                               log.data(), &log_size, &group);
            if (result != OPTIX_SUCCESS)
                throw std::runtime_error(std::string(name) + ": " + optixGetErrorName(result) + "\n" + log.data());
        };
        OptixProgramGroupDesc raygen_desc{};
        raygen_desc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
        raygen_desc.raygen.module = module;
        raygen_desc.raygen.entryFunctionName = "__raygen__triangle";
        create_group(raygen_desc, raygen, "raygen program");
        OptixProgramGroupDesc miss_desc{};
        miss_desc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
        miss_desc.miss.module = module;
        miss_desc.miss.entryFunctionName = "__miss__background";
        create_group(miss_desc, radiance_miss, "radiance miss program");
        OptixProgramGroupDesc shadow_miss_desc{};
        shadow_miss_desc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
        shadow_miss_desc.miss.module = module;
        shadow_miss_desc.miss.entryFunctionName = "__miss__shadow";
        create_group(shadow_miss_desc, shadow_miss, "shadow miss program");
        OptixProgramGroupDesc hit_desc{};
        hit_desc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        hit_desc.hitgroup.moduleCH = module;
        hit_desc.hitgroup.entryFunctionNameCH = "__closesthit__lit";
        create_group(hit_desc, radiance_hit, "radiance closest-hit program");
        OptixProgramGroupDesc shadow_hit_desc{};
        shadow_hit_desc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        shadow_hit_desc.hitgroup.moduleCH = module;
        shadow_hit_desc.hitgroup.entryFunctionNameCH = "__closesthit__shadow";
        create_group(shadow_hit_desc, shadow_hit, "shadow closest-hit program");

        const std::array<OptixProgramGroup, 5> groups{
            raygen, radiance_miss, shadow_miss, radiance_hit, shadow_hit};
        OptixPipelineLinkOptions link_options{};
        link_options.maxTraceDepth = 4;
        log.fill(0); log_size = log.size();
        check_optix(optixPipelineCreate(context, &compile_options, &link_options, groups.data(),
                                        static_cast<unsigned int>(groups.size()), log.data(), &log_size, &pipeline),
                    "optixPipelineCreate");
        OptixStackSizes stack_sizes{};
        for (auto group : groups)
            check_optix(optixUtilAccumulateStackSizes(group, &stack_sizes, pipeline),
                        "optixUtilAccumulateStackSizes");
        unsigned int direct_traversal = 0, direct_state = 0, continuation = 0;
        check_optix(optixUtilComputeStackSizes(&stack_sizes, 4, 0, 0, &direct_traversal,
                                               &direct_state, &continuation), "optixUtilComputeStackSizes");
        check_optix(optixPipelineSetStackSize(pipeline, direct_traversal, direct_state, continuation, 1),
                    "optixPipelineSetStackSize");

        auto upload_records = [](const std::vector<OptixProgramGroup>& groups_to_upload,
                                 CUdeviceptr& destination) {
            std::vector<EmptyRecord> records(groups_to_upload.size());
            for (std::size_t i = 0; i < groups_to_upload.size(); ++i)
                check_optix(optixSbtRecordPackHeader(groups_to_upload[i], &records[i]),
                            "optixSbtRecordPackHeader");
            const std::size_t bytes = records.size() * sizeof(EmptyRecord);
            check_cuda(cudaMalloc(reinterpret_cast<void**>(&destination), bytes), "cudaMalloc SBT records");
            check_cuda(cudaMemcpy(reinterpret_cast<void*>(destination), records.data(), bytes,
                                  cudaMemcpyHostToDevice), "cudaMemcpy SBT record");
        };
        upload_records({raygen}, raygen_record);
        upload_records({radiance_miss, shadow_miss}, miss_record);
        upload_records({radiance_hit, shadow_hit}, hit_record);
        OptixShaderBindingTable sbt{};
        sbt.raygenRecord = raygen_record;
        sbt.missRecordBase = miss_record;
        sbt.missRecordStrideInBytes = sizeof(EmptyRecord);
        sbt.missRecordCount = OPTIX_RAY_TYPE_COUNT;
        sbt.hitgroupRecordBase = hit_record;
        sbt.hitgroupRecordStrideInBytes = sizeof(EmptyRecord);
        sbt.hitgroupRecordCount = OPTIX_RAY_TYPE_COUNT;

        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_image), width * height * sizeof(uchar4)),
                   "cudaMalloc image");
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_accumulation), width * height * sizeof(float4)),
                   "cudaMalloc accumulation");
        check_cuda(cudaMemset(reinterpret_cast<void*>(device_accumulation), 0,
                              width * height * sizeof(float4)), "cudaMemset accumulation");
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_colors), mesh.colors.size() * sizeof(float3)),
                   "cudaMalloc primitive colors");
        check_cuda(cudaMemcpy(reinterpret_cast<void*>(device_colors), mesh.colors.data(),
                              mesh.colors.size() * sizeof(float3), cudaMemcpyHostToDevice),
                   "cudaMemcpy primitive colors");
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_reflectivity),
                              mesh.reflectivity.size() * sizeof(float)),
                   "cudaMalloc primitive reflectivity");
        check_cuda(cudaMemcpy(reinterpret_cast<void*>(device_reflectivity), mesh.reflectivity.data(),
                              mesh.reflectivity.size() * sizeof(float), cudaMemcpyHostToDevice),
                   "cudaMemcpy primitive reflectivity");
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_roughness),
                              mesh.roughness.size() * sizeof(float)),
                   "cudaMalloc primitive roughness");
        check_cuda(cudaMemcpy(reinterpret_cast<void*>(device_roughness), mesh.roughness.data(),
                              mesh.roughness.size() * sizeof(float), cudaMemcpyHostToDevice),
                   "cudaMemcpy primitive roughness");
        float3 minimum = make_float3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                                     std::numeric_limits<float>::max());
        float3 maximum = make_float3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                                     std::numeric_limits<float>::lowest());
        for (const float3 vertex : mesh.vertices) {
            minimum.x = std::min(minimum.x, vertex.x); minimum.y = std::min(minimum.y, vertex.y);
            minimum.z = std::min(minimum.z, vertex.z); maximum.x = std::max(maximum.x, vertex.x);
            maximum.y = std::max(maximum.y, vertex.y); maximum.z = std::max(maximum.z, vertex.z);
        }
        const float3 scene_center = make_float3((minimum.x + maximum.x) * 0.5f,
                                                (minimum.y + maximum.y) * 0.5f,
                                                (minimum.z + maximum.z) * 0.5f);
        const float view_scale = std::max(0.001f, std::max(maximum.x - minimum.x,
                                                          maximum.y - minimum.y) * 0.65f);
        const float3 camera_origin = make_float3(scene_center.x, scene_center.y,
                                                 maximum.z + 2.5f * view_scale);
        const float3 light_position = make_float3(scene_center.x - 1.25f * view_scale,
                                                  scene_center.y + 1.75f * view_scale,
                                                  maximum.z + 2.0f * view_scale);
        OptixTriangleParams params{reinterpret_cast<uchar4*>(device_image),
                                         reinterpret_cast<float4*>(device_accumulation), width, height, 0,
                                         gas_handle,
                                         reinterpret_cast<float3*>(device_vertices),
                                         reinterpret_cast<uint3*>(device_indices),
                                         reinterpret_cast<float3*>(device_colors),
                                         reinterpret_cast<float*>(device_reflectivity),
                                         reinterpret_cast<float*>(device_roughness), camera_origin, view_scale,
                                         light_position};
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_params), sizeof(params)), "cudaMalloc params");
        for (unsigned int sample = 0; sample < samples_per_pixel; ++sample) {
            params.sample_index = sample;
            check_cuda(cudaMemcpy(reinterpret_cast<void*>(device_params), &params, sizeof(params),
                                  cudaMemcpyHostToDevice), "cudaMemcpy params");
            check_optix(optixLaunch(pipeline, nullptr, device_params, sizeof(params), &sbt,
                                    width, height, 1), "optixLaunch");
        }
        check_cuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize");
        float4 center_accumulation{};
        const std::size_t center_offset = ((height / 2) * width + width / 2) * sizeof(float4);
        check_cuda(cudaMemcpy(&center_accumulation,
                              reinterpret_cast<void*>(device_accumulation + center_offset),
                              sizeof(center_accumulation), cudaMemcpyDeviceToHost),
                   "cudaMemcpy center accumulation");
        if (center_accumulation.w != static_cast<float>(samples_per_pixel))
            throw std::runtime_error("progressive accumulation sample count mismatch");
        std::vector<uchar4> image(width * height);
        check_cuda(cudaMemcpy(image.data(), reinterpret_cast<void*>(device_image), image.size() * sizeof(uchar4),
                              cudaMemcpyDeviceToHost), "cudaMemcpy image");
        const uchar4 center_pixel = image[(height / 2) * width + width / 2];
        const std::size_t hit_pixels = static_cast<std::size_t>(std::count_if(
            image.begin(), image.end(), [](uchar4 pixel) {
                return pixel.x != 7 || pixel.y != 12 || pixel.z != 25;
            }));
        if (hit_pixels == 0) throw std::runtime_error("rendered mesh produced no visible pixels");
        const std::size_t shadow_pixels = static_cast<std::size_t>(std::count_if(
            image.begin(), image.end(), [](uchar4 pixel) {
                return pixel.x >= 14 && pixel.x <= 25 && pixel.y >= 14 && pixel.y <= 25 &&
                       pixel.z >= 14 && pixel.z <= 30 &&
                       std::abs(static_cast<int>(pixel.x) - static_cast<int>(pixel.y)) <= 3;
            }));
        if (argc <= 2 && shadow_pixels == 0)
            throw std::runtime_error("default scene produced no verified shadow pixels");
        write_ppm(output_path, image, width, height);
        std::cout << "OptiX indexed mesh rendered " << mesh.indices.size() << " triangles to "
                  << output_path << " at " << samples_per_pixel << " spp (" << hit_pixels
                  << " hit pixels, " << shadow_pixels
                  << " verified shadow pixels, center RGB "
                  << static_cast<int>(center_pixel.x) << ", " << static_cast<int>(center_pixel.y) << ", "
                  << static_cast<int>(center_pixel.z) << ")\n";

        cudaFree(reinterpret_cast<void*>(device_params));
        cudaFree(reinterpret_cast<void*>(device_roughness));
        cudaFree(reinterpret_cast<void*>(device_accumulation));
        cudaFree(reinterpret_cast<void*>(device_reflectivity));
        cudaFree(reinterpret_cast<void*>(device_colors));
        cudaFree(reinterpret_cast<void*>(device_indices));
        cudaFree(reinterpret_cast<void*>(device_vertices));
        cudaFree(reinterpret_cast<void*>(device_image));
        cudaFree(reinterpret_cast<void*>(hit_record));
        cudaFree(reinterpret_cast<void*>(miss_record));
        cudaFree(reinterpret_cast<void*>(raygen_record));
        cudaFree(reinterpret_cast<void*>(gas));
        optixPipelineDestroy(pipeline);
        optixProgramGroupDestroy(shadow_hit);
        optixProgramGroupDestroy(radiance_hit);
        optixProgramGroupDestroy(shadow_miss);
        optixProgramGroupDestroy(radiance_miss);
        optixProgramGroupDestroy(raygen);
        optixModuleDestroy(module);
        optixDeviceContextDestroy(context);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "OptiX triangle demo failed: " << error.what() << '\n';
        return 1;
    }
}
