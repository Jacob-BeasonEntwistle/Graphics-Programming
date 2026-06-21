#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <lodepng.h>

#include "TriangleRenderer.hpp"
#include "MeshRenderer.hpp"
#include "Material.hpp"
#include "ChromaticAberration.hpp"

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

// --[CREATING MATERIALS]--
// --Wood--
Material* bookshelfMaterial = new Wood(
	Eigen::Vector3f(0.08f, 0.08f, 0.08f),		// Specular color
	80.0f										// Specular exponent
);
Material* tvStandMaterial = new Wood(
	Eigen::Vector3f(0.08f, 0.08f, 0.08f),		// Specular color
	80.0f										// Specular exponent
);
Material* coffeeTableMaterial = new Wood(
	Eigen::Vector3f(0.08f, 0.08f, 0.08f),		// Specular color
	80.0f										// Specular exponent
);
Material* windowFrameMaterial = new Wood(
	Eigen::Vector3f(0.08f, 0.08f, 0.08f),		// Specular color
	80.0f										// Specular exponent
);
Material* shelfMaterial = new Wood(
	Eigen::Vector3f(0.08f, 0.08f, 0.08f),		// Specular color
	80.0f										// Specular exponent
);

// --Glass--
Material* windowPaneMaterial = new Glass(
	Eigen::Vector3f(1.0f, 1.0f, 1.0f),			// Specular color
	300.0f										// Specular exponent
);
Material* tvMaterial = new Glass(
	Eigen::Vector3f(0.009f, 0.007f, 0.005f),	// Specular color
	200.0f										// Specular exponent
);

// --Other--
Material* roomMaterial = new Plaster(
	Eigen::Vector3f(1.0f, 1.0f, 1.0f),			// Specular color
	20.0f										// Specular exponent
);
Material* sofaMaterial = new Fabric();
Material* floorMaterial = new Carpet();

// "Why no albedo in materials?" - Materials describe how light interacts with the surface, textures describe the surface's albedo

int main()
{
	std::string outputFilename = "output.png";

	const int width = 1920, height = 1080;
	const int nChannels = 4;

	// Set up an image buffer
	std::vector<uint8_t> imageBuffer(height * width * nChannels);
	std::vector<uint8_t> imageBufferChromatic(height * width * nChannels);
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
	Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(0.0f, 0.5f, 1.0f)) * rotateXMatrix(0.13f);

	Eigen::Vector3f camWorldPos = (cameraToWorld * Eigen::Vector4f(0, 0, 0, 1)).block<3, 1>(0, 0);

	// The main important task = set up the worldToCamera and worldToClip matrices here!
	// Set up worldToCamera, based on cameraToWorld above
	Eigen::Matrix4f worldToCamera = cameraToWorld.inverse();
	// Set up worldToClip, using the projection and worldToCamera matrices
	Eigen::Matrix4f worldToClip = projection * worldToCamera;



	// --[SETTING THE FILENAMES OF EACH OBJECT]--
	std::string roomFilename = "../../../models/Room.obj";
	std::string floorFilename = "../../../models/Floor.obj";
	std::string bookshelfFilename = "../../../models/Bookshelf.obj";
	std::string coffeetableFilename = "../../../models/CoffeeTable.obj";
	std::string shelfFilename = "../../../models/Shelf.obj";
	std::string sofaFilename = "../../../models/Sofa.obj";
	std::string tvFilename = "../../../models/TV.obj";
	std::string tvstandFilename = "../../../models/TVStand.obj";
	std::string windowframeFilename = "../../../models/WindowFrame.obj";
	std::string windowpaneFilename = "../../../models/WindowPane.obj";


	// --[CREATING THE TEXTURES FOR EACH OBJECT]--
	std::vector<uint8_t> leatherTexture;
	unsigned int leatherTexWidth, leatherTexHeight;
	lodepng::decode(leatherTexture, leatherTexWidth, leatherTexHeight, "../../../textures/Leather030_Color.png");

	std::vector<uint8_t> carpetTexture;
	unsigned int carpetTexWidth, carpetTexHeight;
	lodepng::decode(carpetTexture, carpetTexWidth, carpetTexHeight, "../../../textures/Carpet016_Color.png");

	std::vector<uint8_t> plasterTexture;
	unsigned int plasterTexWidth, plasterTexHeight;
	lodepng::decode(plasterTexture, plasterTexWidth, plasterTexHeight, "../../../textures/Plaster001_Color.png");

	std::vector<uint8_t> woodLightTexture;
	unsigned int woodLightTexWidth, woodLightTexHeight;
	lodepng::decode(woodLightTexture, woodLightTexWidth, woodLightTexHeight, "../../../textures/Wood095_Color.png");

	std::vector<uint8_t> woodDarkTexture;
	unsigned int woodDarkTexWidth, woodDarkTexHeight;
	lodepng::decode(woodDarkTexture, woodDarkTexWidth, woodDarkTexHeight, "../../../textures/Wood066_Color.png");


	// --[CREATING THE LIGHTS]--
	std::vector<std::unique_ptr<Light>> lights;
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(0.02f, 0.02f, 0.02f)));
	lights.emplace_back(new PointLight(Eigen::Vector3f(1.6f, 1.6f, 1.6f), Eigen::Vector3f(0.0f, 2.0f, 5.0f)));



	// --[LOADING THE MESHES]--
	Mesh roomMesh = loadMeshFile(roomFilename);
	Mesh floorMesh = loadMeshFile(floorFilename);
	Mesh bookshelfMesh = loadMeshFile(bookshelfFilename);
	Mesh coffeetableMesh = loadMeshFile(coffeetableFilename);
	Mesh shelfMesh = loadMeshFile(shelfFilename);
	Mesh sofaMesh = loadMeshFile(sofaFilename);
	Mesh tvMesh = loadMeshFile(tvFilename);
	Mesh tvstandMesh = loadMeshFile(tvstandFilename);
	Mesh windowframeMesh = loadMeshFile(windowframeFilename);
	Mesh windowpaneMesh = loadMeshFile(windowpaneFilename);

	// Where in space to put the scene
	Eigen::Vector3f sceneOrigin(0.0f, -1.0f, 5.0f);

	// --[POSITIONING THE MESHES]--
	Eigen::Matrix4f roomTransform;
	roomTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	// Plug the earlier created material into the drawMesh(...) function
	drawMesh(imageBuffer, zBuffer, roomMesh, roomMaterial, camWorldPos,
		roomTransform, worldToClip, lights, width, height, plasterTexture, plasterTexWidth, plasterTexHeight);

	Eigen::Matrix4f floorTransform;
	floorTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, floorMesh, floorMaterial, camWorldPos,
		floorTransform, worldToClip, lights, width, height, carpetTexture, carpetTexWidth, carpetTexHeight);

	Eigen::Matrix4f bookshelfTransform;
	bookshelfTransform = translationMatrix(sceneOrigin - Eigen::Vector3f(0.05f, 0.0f, 0.0f)) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, bookshelfMesh, bookshelfMaterial, camWorldPos,
		bookshelfTransform, worldToClip, lights, width, height, woodDarkTexture, woodDarkTexWidth, woodDarkTexHeight);

	Eigen::Matrix4f coffeetableTransform;
	coffeetableTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, coffeetableMesh, coffeeTableMaterial, camWorldPos,
		coffeetableTransform, worldToClip, lights, width, height, woodDarkTexture, woodDarkTexWidth, woodDarkTexHeight);

	Eigen::Matrix4f shelfTransform;
	shelfTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, shelfMesh, shelfMaterial, camWorldPos,
		shelfTransform, worldToClip, lights, width, height, woodLightTexture, woodLightTexWidth, woodLightTexHeight);

	Eigen::Matrix4f sofaTransform;
	sofaTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, sofaMesh, sofaMaterial, camWorldPos,
		sofaTransform, worldToClip, lights, width, height, leatherTexture, leatherTexWidth, leatherTexHeight);

	Eigen::Matrix4f tvTransform;
	tvTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, tvMesh, tvMaterial, camWorldPos,
		tvTransform, worldToClip, lights, width, height, plasterTexture, plasterTexWidth, plasterTexHeight);

	Eigen::Matrix4f tvStandTransform;
	tvStandTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, tvstandMesh, tvStandMaterial, camWorldPos,
		tvStandTransform, worldToClip, lights, width, height, woodLightTexture, woodLightTexWidth, woodLightTexHeight);

	Eigen::Matrix4f windowframeTransform;
	windowframeTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, windowframeMesh, windowFrameMaterial, camWorldPos,
		windowframeTransform, worldToClip, lights, width, height, woodLightTexture, woodLightTexWidth, woodLightTexHeight);

	Eigen::Matrix4f windowpaneTransform;
	windowpaneTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, windowpaneMesh, windowPaneMaterial, camWorldPos,
		windowpaneTransform, worldToClip, lights, width, height, plasterTexture, plasterTexWidth, plasterTexHeight);

	// For debug - draw point lights as colored circles so we can see where they are
	drawPointLights(imageBuffer, width, height, lights);

	// Delete the created materials to prevent memory leak
	// --Wood--
	delete bookshelfMaterial;
	delete tvStandMaterial;
	delete coffeeTableMaterial;
	delete windowFrameMaterial;
	delete shelfMaterial;

	// --Glass--
	delete windowPaneMaterial;
	delete tvMaterial;

	// --Other--
	delete roomMaterial;
	delete sofaMaterial;
	delete floorMaterial;

	ChromaticAberration ca(500.0f, 1.0f, 0.95f, 0.9f);
	auto result = ca.applyAberration(imageBuffer, imageBufferChromatic, width, height, nChannels);

    // Save the image
    int errorCode;
        errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
        if (errorCode) { // check the error code, in case an error occurred.
            std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
            return errorCode;
        }
		errorCode = lodepng::encode("output_aberrated.png", imageBufferChromatic, width, height);
		if (errorCode) { // check the error code, in case an error occurred.
			std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
			return errorCode;
		}

    return 0;
}
