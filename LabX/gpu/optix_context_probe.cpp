#include <cuda.h>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void check_cuda(CUresult result, const char* operation) {
    if (result == CUDA_SUCCESS) return;
    const char* message = nullptr;
    cuGetErrorString(result, &message);
    throw std::runtime_error(std::string(operation) + ": " + (message ? message : "unknown CUDA driver error"));
}

void check_optix(OptixResult result, const char* operation) {
    if (result != OPTIX_SUCCESS)
        throw std::runtime_error(std::string(operation) + ": OptiX error " + std::to_string(result));
}
}

int main() {
    OptixDeviceContext context = nullptr;
    CUdevice device = 0;
    CUcontext cuda_context = nullptr;
    try {
        check_cuda(cuInit(0), "cuInit");
        check_optix(optixInit(), "optixInit");
        check_cuda(cuDeviceGet(&device, 0), "cuDeviceGet");
        check_cuda(cuDevicePrimaryCtxRetain(&cuda_context, device), "cuDevicePrimaryCtxRetain");
        OptixDeviceContextOptions options{};
        check_optix(optixDeviceContextCreate(cuda_context, &options, &context), "optixDeviceContextCreate");
        unsigned int maximum_trace_depth = 0;
        check_optix(optixDeviceContextGetProperty(context, OPTIX_DEVICE_PROPERTY_LIMIT_MAX_TRACE_DEPTH,
                                                   &maximum_trace_depth, sizeof(maximum_trace_depth)),
                    "OPTIX_DEVICE_PROPERTY_LIMIT_MAX_TRACE_DEPTH");
        std::cout << "OptiX context created; maximum trace depth " << maximum_trace_depth << '\n';
        check_optix(optixDeviceContextDestroy(context), "optixDeviceContextDestroy");
        check_cuda(cuDevicePrimaryCtxRelease(device), "cuDevicePrimaryCtxRelease");
        return 0;
    } catch (const std::exception& error) {
        if (context) optixDeviceContextDestroy(context);
        if (cuda_context) cuDevicePrimaryCtxRelease(device);
        std::cerr << "OptiX context probe failed: " << error.what() << '\n';
        return 1;
    }
}
