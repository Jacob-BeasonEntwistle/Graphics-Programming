#pragma once
#include "LinAlg.hpp"
#include <array>

struct Triangle {
	std::array<Eigen::Vector3f, 3> screen;		// Coordinates of the triangle in screen space.
	std::array<Eigen::Vector3f, 3> verts;		// Vertices of the triangle in world space.
	std::array<Eigen::Vector3f, 3> norms;		// Normals of the triangle corners in world space.
	std::array<Eigen::Vector2f, 3> texs;		// Texture coordinates of the triangle corners.
};
