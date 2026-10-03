cmake_minimum_required(VERSION 3.18)

project(cpu_rasterization_labs LANGUAGES CXX)

include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_LEGACY_LABS "Build the original OpenGL/GLUT lab applications" ON)
option(BUILD_CUDA_DEMOS "Build CUDA experiments under LabX/gpu" OFF)
set(CG_CUDA_ARCHITECTURES "89" CACHE STRING "CUDA architectures for LabX GPU experiments")

add_library(cg_core
    LabX/pbr/pbr.cpp
    LabX/progressive/progressive_renderer.cpp
    src/animation/animation.cpp
    src/core/draw.cpp
    src/core/image.cpp
    src/curves/bezier.cpp
    src/raytracer/raytracer.cpp
)
target_include_directories(cg_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/LabX")
target_compile_features(cg_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(cg_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(cg_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(core_tests tests/core_tests.cpp)
    target_link_libraries(core_tests PRIVATE cg_core)
    add_test(NAME core_tests COMMAND core_tests)

    add_executable(bezier_tests tests/bezier_tests.cpp)
    target_link_libraries(bezier_tests PRIVATE cg_core)
    add_test(NAME bezier_tests COMMAND bezier_tests)

    add_executable(raytracer_tests tests/raytracer_tests.cpp)
    target_link_libraries(raytracer_tests PRIVATE cg_core)
    add_test(NAME raytracer_tests COMMAND raytracer_tests)

    add_executable(pbr_tests LabX/pbr/pbr_tests.cpp)
    target_link_libraries(pbr_tests PRIVATE cg_core)
    add_test(NAME pbr_tests COMMAND pbr_tests)

    add_executable(progressive_tests LabX/progressive/progressive_tests.cpp)
    target_link_libraries(progressive_tests PRIVATE cg_core)
    add_test(NAME progressive_tests COMMAND progressive_tests)

    add_executable(animation_tests tests/animation_tests.cpp)
    target_link_libraries(animation_tests PRIVATE cg_core)
    add_test(NAME animation_tests COMMAND animation_tests)
endif()

add_executable(bezier_demo LabX/curves/bezier_demo.cpp)
target_link_libraries(bezier_demo PRIVATE cg_core)

add_executable(raytracer_demo LabX/raytracing/raytracer_demo.cpp)
target_link_libraries(raytracer_demo PRIVATE cg_core)

add_executable(pathtracer_demo LabX/raytracing/pathtracer_demo.cpp)
target_link_libraries(pathtracer_demo PRIVATE cg_core)

add_executable(animation_demo LabX/animation/animation_demo.cpp)
target_link_libraries(animation_demo PRIVATE cg_core)

if(BUILD_CUDA_DEMOS)
    include(CheckLanguage)
    check_language(CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
        message(FATAL_ERROR "BUILD_CUDA_DEMOS=ON, but no CUDA compiler was found")
    endif()
    enable_language(CUDA)
    add_executable(cuda_capability LabX/gpu/cuda_capability.cu)
    set_target_properties(cuda_capability PROPERTIES
        CUDA_ARCHITECTURES "${CG_CUDA_ARCHITECTURES}"
        CUDA_STANDARD 17
        CUDA_STANDARD_REQUIRED ON
    )
    if(MSVC AND CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 12.0)
        target_compile_options(cuda_capability PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-allow-unsupported-compiler>)
    endif()
    if(BUILD_TESTING)
        add_test(NAME cuda_capability COMMAND cuda_capability)
    endif()
endif()

if(BUILD_LEGACY_LABS)
find_package(OpenGL REQUIRED)
find_package(GLUT QUIET)

if(GLUT_FOUND)
    set(GLUT_TARGET GLUT::GLUT)
else()
    set(FREEGLUT_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(FREEGLUT_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        freeglut
        URL https://github.com/freeglut/freeglut/archive/refs/tags/v3.8.0.zip
        URL_HASH SHA256=66c12fbf41ef5da34a386d59dbd389e30b601124937f8edb81a30a5006247f93
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(freeglut)
    set(GLUT_TARGET freeglut_static)
    set(GLUT_INCLUDE_DIR "${freeglut_SOURCE_DIR}/include")
endif()

function(add_graphics_lab target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE OpenGL::GL OpenGL::GLU ${GLUT_TARGET})

    if(GLUT_TARGET STREQUAL "freeglut_static")
        target_compile_definitions(${target} PRIVATE FREEGLUT_STATIC)
        target_include_directories(${target} PRIVATE "${GLUT_INCLUDE_DIR}")
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_graphics_lab(lab2 2022CG_Lab2/2022CG_Lab2.cpp)
add_graphics_lab(lab3 2022CG_Lab3/2022CG_Lab3.cpp)
add_graphics_lab(lab4 2022CG_Lab4/2022CG_Lab4.cpp)
add_graphics_lab(bezier_interactive LabX/curves/bezier_interactive.cpp)
target_link_libraries(bezier_interactive PRIVATE cg_core)

set_target_properties(lab2 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/2022CG_Lab2")
set_target_properties(lab3 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/2022CG_Lab3")
set_target_properties(lab4 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/2022CG_Lab4")
endif()
