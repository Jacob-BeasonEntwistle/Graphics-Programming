cmake_minimum_required(VERSION 3.27.1)

project(rasteriser)

add_subdirectory(3rdParty)

include_directories(3rdParty/lodepng)
include_directories(../../3rdParty/eigen-3.4.0)

add_executable(rasteriser
    source/rasteriser.cpp
    source/Camera.hpp
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
    )