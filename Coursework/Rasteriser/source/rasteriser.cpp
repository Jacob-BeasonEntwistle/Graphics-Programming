#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <lodepng.h>

#include "TriangleRenderer.hpp"
#include "MeshRenderer.hpp"
#include "Material.hpp"

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
Material* woodMaterial = new Wood(
	Eigen::Vector3f(0.8f, 0.5f, 0.3f),		// Albedo
	Eigen::Vector3f(0.2f, 0.2f, 0.2f),		// Specular color
	50.0f									// Specular exponent
);

Material* glassMaterial = new Glass(
	Eigen::Vector3f(0.9f, 0.9f, 1.0f),		// Specular color
	100.0f									// Specular exponent
);

Material* fabricMaterial = new Fabric(
	Eigen::Vector3f(0.6f, 0.1f, 0.1f)		// Albedo
);

Material* plasterMaterial = new Plaster(
	Eigen::Vector3f(0.75f, 0.73f, 0.7f),	// Albedo
	Eigen::Vector3f(0.3f, 0.3f, 0.3f),		// Specular color
	20.0f									// Specular exponent
);

Material* carpetMaterial = new Carpet(
	Eigen::Vector3f(0.25f, 0.3f, 0.45f)
);

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
	Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(0.f, 0.8f, 0.0f)) * rotateXMatrix(0.26f);

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
	std::string coffeetableFilename = "../../../models/Coffee_table.obj";
	std::string shelfFilename = "../../../models/Shelf.obj";
	std::string sofaFilename = "../../../models/Sofa.obj";
	std::string tvFilename = "../../../models/TV.obj";
	std::string tvstandFilename = "../../../models/TV_stand.obj";
	std::string windowframeFilename = "../../../models/Window_frame.obj";
	std::string windowpaneFilename = "../../../models/Window_pane.obj";



	// --[CREATING THE LIGHTS]--
	std::vector<std::unique_ptr<Light>> lights;
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(0.2f, 0.2f, 0.2f)));
	lights.emplace_back(new PointLight(Eigen::Vector3f(2.5f, 2.5f, 2.5f), Eigen::Vector3f(0.0f, 2.0f, 5.0f)));



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
	drawMesh(imageBuffer, zBuffer, roomMesh, plasterMaterial, camWorldPos,
		roomTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f floorTransform;
	floorTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, floorMesh, carpetMaterial, camWorldPos,
		floorTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f bookshelfTransform;
	bookshelfTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	// Plug the earlier created material into the drawMesh(...) function
	drawMesh(imageBuffer, zBuffer, bookshelfMesh, woodMaterial, camWorldPos,
		bookshelfTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f coffeetableTransform;
	coffeetableTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, coffeetableMesh, woodMaterial, camWorldPos,
		coffeetableTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f shelfTransform;
	shelfTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, shelfMesh, woodMaterial, camWorldPos,
		shelfTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f sofaTransform;
	sofaTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, sofaMesh, fabricMaterial, camWorldPos,
		sofaTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f tvTransform;
	tvTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, tvMesh, glassMaterial, camWorldPos,
		tvTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f tvstandTransform;
	tvstandTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, tvstandMesh, woodMaterial, camWorldPos,
		tvstandTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f windowframeTransform;
	windowframeTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, windowframeMesh, woodMaterial, camWorldPos,
		windowframeTransform, worldToClip, lights, width, height);

	Eigen::Matrix4f windowpaneTransform;
	windowpaneTransform = translationMatrix(sceneOrigin) * rotateYMatrix(M_PI) * scaleMatrix(1.0f);
	drawMesh(imageBuffer, zBuffer, windowpaneMesh, glassMaterial, camWorldPos,
		windowpaneTransform, worldToClip, lights, width, height);

	// For debug - draw point lights as colored circles so we can see where they are
	drawPointLights(imageBuffer, width, height, lights);

	// Delete the created materials to prevent memory leak
	delete woodMaterial;
	delete glassMaterial;
	delete fabricMaterial;
	delete plasterMaterial;
	delete carpetMaterial;

    // Save the image
    int errorCode;
        errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
        if (errorCode) { // check the error code, in case an error occurred.
            std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
            return errorCode;
        }

    return 0;
}
