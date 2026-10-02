cmake_minimum_required(VERSION 3.16)

project(cpu_rasterization_labs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

function(add_graphics_lab target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE OpenGL::GL OpenGL::GLU GLUT::GLUT)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_graphics_lab(lab2 2022CG_Lab2/2022CG_Lab2.cpp)
add_graphics_lab(lab3 2022CG_Lab3/2022CG_Lab3.cpp)
add_graphics_lab(lab4 2022CG_Lab4/2022CG_Lab4.cpp)

set_target_properties(lab2 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/2022CG_Lab2")
set_target_properties(lab3 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/2022CG_Lab3")
set_target_properties(lab4 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/2022CG_Lab4")
