cmake_minimum_required(VERSION 3.27.1)

project(rasteriser)

add_subdirectory(3rdParty)

include_directories(3rdParty/lodepng)
include_directories(../assets/3rdParty/eigen-3.4.0)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(rasteriser
    source/rasteriser.cpp
    source/Image.hpp
    source/Light.hpp
    source/LinAlg.hpp
    source/Mesh.hpp
    source/MeshRenderer.hpp
    source/Shading.hpp
    source/Triangle.hpp
    source/TriangleRenderer.hpp
    source/Material.hpp
    source/ChromaticAberration.hpp
)

target_link_libraries(rasteriser 
    lodepng
    ${OpenCV_LIBS}
    )