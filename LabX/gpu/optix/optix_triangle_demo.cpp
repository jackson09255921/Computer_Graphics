#include <cuda_runtime.h>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stack_size.h>
#include <optix_stubs.h>

#include "optix_triangle_shared.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
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
    const std::string output_path = argc > 1 ? argv[1] : "optix_triangle.ppm";

    OptixDeviceContext context = nullptr;
    OptixModule module = nullptr;
    OptixPipeline pipeline = nullptr;
    OptixProgramGroup raygen = nullptr, miss = nullptr, hitgroup = nullptr;
    CUdeviceptr gas = 0, raygen_record = 0, miss_record = 0, hit_record = 0;
    CUdeviceptr device_image = 0, device_params = 0;

    try {
        check_cuda(cudaFree(nullptr), "cudaFree initialization");
        check_optix(optixInit(), "optixInit");
        OptixDeviceContextOptions context_options{};
        check_optix(optixDeviceContextCreate(nullptr, &context_options, &context),
                    "optixDeviceContextCreate");

        const std::array<float3, 3> vertices{{
            make_float3(-0.75f, -0.60f, 0.0f),
            make_float3( 0.75f, -0.60f, 0.0f),
            make_float3( 0.00f,  0.75f, 0.0f)}};
        CUdeviceptr device_vertices = 0;
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_vertices), sizeof(vertices)), "cudaMalloc vertices");
        check_cuda(cudaMemcpy(reinterpret_cast<void*>(device_vertices), vertices.data(), sizeof(vertices),
                              cudaMemcpyHostToDevice), "cudaMemcpy vertices");
        const unsigned int geometry_flags[] = {OPTIX_GEOMETRY_FLAG_NONE};
        OptixBuildInput build_input{};
        build_input.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
        build_input.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
        build_input.triangleArray.numVertices = 3;
        build_input.triangleArray.vertexBuffers = &device_vertices;
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
        check_cuda(cudaFree(reinterpret_cast<void*>(device_vertices)), "cudaFree vertices");

        OptixPipelineCompileOptions compile_options{};
        compile_options.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
        compile_options.numPayloadValues = 3;
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
        create_group(miss_desc, miss, "miss program");
        OptixProgramGroupDesc hit_desc{};
        hit_desc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        hit_desc.hitgroup.moduleCH = module;
        hit_desc.hitgroup.entryFunctionNameCH = "__closesthit__barycentric";
        create_group(hit_desc, hitgroup, "closest-hit program");

        const std::array<OptixProgramGroup, 3> groups{raygen, miss, hitgroup};
        OptixPipelineLinkOptions link_options{};
        link_options.maxTraceDepth = 1;
        log.fill(0); log_size = log.size();
        check_optix(optixPipelineCreate(context, &compile_options, &link_options, groups.data(),
                                        static_cast<unsigned int>(groups.size()), log.data(), &log_size, &pipeline),
                    "optixPipelineCreate");
        OptixStackSizes stack_sizes{};
        for (auto group : groups)
            check_optix(optixUtilAccumulateStackSizes(group, &stack_sizes, pipeline),
                        "optixUtilAccumulateStackSizes");
        unsigned int direct_traversal = 0, direct_state = 0, continuation = 0;
        check_optix(optixUtilComputeStackSizes(&stack_sizes, 1, 0, 0, &direct_traversal,
                                               &direct_state, &continuation), "optixUtilComputeStackSizes");
        check_optix(optixPipelineSetStackSize(pipeline, direct_traversal, direct_state, continuation, 1),
                    "optixPipelineSetStackSize");

        auto upload_record = [](OptixProgramGroup group, CUdeviceptr& destination) {
            EmptyRecord record{};
            check_optix(optixSbtRecordPackHeader(group, &record), "optixSbtRecordPackHeader");
            check_cuda(cudaMalloc(reinterpret_cast<void**>(&destination), sizeof(record)), "cudaMalloc SBT record");
            check_cuda(cudaMemcpy(reinterpret_cast<void*>(destination), &record, sizeof(record),
                                  cudaMemcpyHostToDevice), "cudaMemcpy SBT record");
        };
        upload_record(raygen, raygen_record);
        upload_record(miss, miss_record);
        upload_record(hitgroup, hit_record);
        OptixShaderBindingTable sbt{};
        sbt.raygenRecord = raygen_record;
        sbt.missRecordBase = miss_record;
        sbt.missRecordStrideInBytes = sizeof(EmptyRecord);
        sbt.missRecordCount = 1;
        sbt.hitgroupRecordBase = hit_record;
        sbt.hitgroupRecordStrideInBytes = sizeof(EmptyRecord);
        sbt.hitgroupRecordCount = 1;

        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_image), width * height * sizeof(uchar4)),
                   "cudaMalloc image");
        const OptixTriangleParams params{reinterpret_cast<uchar4*>(device_image), width, height, gas_handle};
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&device_params), sizeof(params)), "cudaMalloc params");
        check_cuda(cudaMemcpy(reinterpret_cast<void*>(device_params), &params, sizeof(params),
                              cudaMemcpyHostToDevice), "cudaMemcpy params");
        check_optix(optixLaunch(pipeline, nullptr, device_params, sizeof(params), &sbt, width, height, 1),
                    "optixLaunch");
        check_cuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize");
        std::vector<uchar4> image(width * height);
        check_cuda(cudaMemcpy(image.data(), reinterpret_cast<void*>(device_image), image.size() * sizeof(uchar4),
                              cudaMemcpyDeviceToHost), "cudaMemcpy image");
        const uchar4 center = image[(height / 2) * width + width / 2];
        if (center.x < 16 && center.y < 16 && center.z < 32)
            throw std::runtime_error("center pixel missed the triangle");
        write_ppm(output_path, image, width, height);
        std::cout << "OptiX triangle rendered to " << output_path << " (center RGB "
                  << static_cast<int>(center.x) << ", " << static_cast<int>(center.y) << ", "
                  << static_cast<int>(center.z) << ")\n";

        cudaFree(reinterpret_cast<void*>(device_params));
        cudaFree(reinterpret_cast<void*>(device_image));
        cudaFree(reinterpret_cast<void*>(hit_record));
        cudaFree(reinterpret_cast<void*>(miss_record));
        cudaFree(reinterpret_cast<void*>(raygen_record));
        cudaFree(reinterpret_cast<void*>(gas));
        optixPipelineDestroy(pipeline);
        optixProgramGroupDestroy(hitgroup);
        optixProgramGroupDestroy(miss);
        optixProgramGroupDestroy(raygen);
        optixModuleDestroy(module);
        optixDeviceContextDestroy(context);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "OptiX triangle demo failed: " << error.what() << '\n';
        return 1;
    }
}

