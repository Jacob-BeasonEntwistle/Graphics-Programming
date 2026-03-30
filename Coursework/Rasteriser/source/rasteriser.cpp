#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <lodepng.h>

#include "TriangleRenderer.hpp"
#include "MeshRenderer.hpp"

Eigen::Matrix4f projectionMatrix(int height, int width, float horzFov = 70.f * M_PI / 180.f, float zFar = 10.f, float zNear = 0.1f)
{
	float vertFov = horzFov * float(height) / width;
	Eigen::Matrix4f projection;
	projection <<
		1.0f / tanf(0.5f * horzFov), 0, 0, 0,
		0, 1.0f / tanf(0.5f * vertFov), 0, 0,
		0, 0, zFar / (zFar - zNear), -zFar * zNear / (zFar - zNear),
		0, 0, 1, 0;
	return projection;
}

int main()
{
	std::string outputFilename = "output.png";

	const int width = 1920, height = 1080;
	const int nChannels = 4;

	// Set up an image buffer
	std::vector<uint8_t> imageBuffer(height*width*nChannels);
	std::vector<float> zBuffer(height * width);

	// This line sets the image to black initially.
	Color black{ 0,0,0,255 };
	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			setPixel(imageBuffer, c, r, width, height, black);
			zBuffer[r * width + c] = 1.0f;
		}
	}



	// --[RASTERISER CODE]--
	Eigen::Matrix4f projection = projectionMatrix(height, width);

	// This matrix rotates the camera, tilting it down, then translates it up to make it look down on the scene.
	Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(0.f, 0.8f, 0.f)) * rotateXMatrix(0.4f);

	Eigen::Vector3f camWorldPos = (cameraToWorld * Eigen::Vector4f(0, 0, 0, 1)).block<3, 1>(0, 0);

	// The main important task = set up the worldToCamera and worldToClip matrices here!
	// Set up worldToCamera, based on cameraToWorld above
	Eigen::Matrix4f worldToCamera = cameraToWorld.inverse();
	// Set up worldToClip, using the projection and worldToCamera matrices
	Eigen::Matrix4f worldToClip = projection * worldToCamera;



	// --[SETTING THE FILENAMES OF EACH OBJECT]--
	std::string bunnyFilename = "../models/stanford_bunny_texmapped.obj";
	std::string planeFilename = "../models/plane.obj";



	// --[CREATING THE LIGHTS]--
	std::vector<std::unique_ptr<Light>> lights;
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(0.1f, 0.1f, 0.1f)));
	lights.emplace_back(new DirectionalLight(Eigen::Vector3f(0.4f, 0.4f, 0.4f), Eigen::Vector3f(1.f, -1.f, 0.0f)));



	// --[LOADING THE MESHES]--
	Mesh bunnyMesh = loadMeshFile(bunnyFilename);
	Mesh planeMesh = loadMeshFile(planeFilename);



	// --[POSITIONING THE MESHES]--
	Eigen::Matrix4f bunnyTransform;
	bunnyTransform = translationMatrix(Eigen::Vector3f(0.0f, -1.0f, 3.f)) * rotateYMatrix(M_PI);
	// .... and change the specular exponent here!
	drawMesh(imageBuffer, zBuffer, bunnyMesh, Eigen::Vector3f(0.f, 0.5f, 0.8f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		bunnyTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f planeTransform;
	planeTransform = translationMatrix(Eigen::Vector3f(0.0f, -1.0f, 3.f)) * scaleMatrix(1.4f);
	drawMesh(imageBuffer, zBuffer, planeMesh, Eigen::Vector3f(0.f, 0.5f, 0.8f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		planeTransform, worldToClip, lights, width, height);

    // Save the image
    int errorCode;
        errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
        if (errorCode) { // check the error code, in case an error occurred.
            std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
            return errorCode;
        }

    return 0;
}
