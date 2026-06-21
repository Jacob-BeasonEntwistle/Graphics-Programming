#pragma once
#include "Ray.hpp"

/// <summary>
/// Movable camera class. Provide the camera location, forward direction and an up
/// vector, along with the image dimensions and vertical Field of View angle (radians).
/// The camera can then produce a ray passing through each pixel location.
/// </summary>
class Camera
{
private:
	Eigen::Vector3f location_, bottomLeftPix_, right1pix_, up1pix_;

public:
	Camera(
		const Eigen::Vector3f& location,
		const Eigen::Vector3f& forward,
		const Eigen::Vector3f& up,
		int pixWidth, int pixHeight,
		float vertFov)
		:location_(location)
	{
		Eigen::Vector3f forwardVec = forward.normalized();
		// Draws the scene left to right instead of right to left (mirrors it on the X axis)
		Eigen::Vector3f rightVec = (forward.cross(up)).normalized();
		Eigen::Vector3f upVec = (rightVec.cross(forward)).normalized();

		float aspect = static_cast<float>(pixWidth) / static_cast<float>(pixHeight);

		float halfHeight = tan(vertFov / 2);
		float halfWidth = aspect * halfHeight;

		bottomLeftPix_ = location + forwardVec - (halfWidth * rightVec + halfHeight * upVec);

		right1pix_ = rightVec * halfWidth * 2.f / static_cast<float>(pixWidth);
		up1pix_ = upVec * halfHeight * 2.f / static_cast<float>(pixHeight);
	}

	Ray getRay(int pixX, int pixY)
	{
		Ray ray;
		ray.origin = location_;
		Eigen::Vector3f pixelPos = bottomLeftPix_ +
			static_cast<float>(pixX) * right1pix_ +
			static_cast<float>(pixY) * up1pix_;

		ray.direction = (pixelPos - location_).normalized();

		// ::DEBUGGING:: to test whether the ray directions are different
		/*static int debugCount = 0;
		if (debugCount < 5) {
			std::cout << "Ray dir: "
				<< ray.direction.x() << ", "
				<< ray.direction.y() << ", "
				<< ray.direction.z() << std::endl;
			debugCount++;
		}*/

		return ray;
	}
};

