#include <Eigen/Dense>
#include <lodepng.h>
#include <json/json.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "BVHNode.hpp"
#include "Triangle.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "PointLight.hpp"
#include "DirectionalLight.hpp"
#include "LambertianShader.hpp"
#include "TexturedLambertianShader.hpp"
#include "PhongShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "Model.hpp"
#include <fstream>

/// <summary>
/// Load a JSON config file using the nlohmann library.
/// </summary>
nlohmann::json loadConfig(const std::string& filename)
{
	std::ifstream configStream(filename);
	nlohmann::json config = nlohmann::json::parse(configStream);
	return config;
}

/// <summary>
/// Load an Eigen Vector3f from a config file.
/// Call as for example loadVec3FromConfig(config["myVector3"]);
/// </summary>
Eigen::Vector3f loadVec3FromConfig(const nlohmann::json& config)
{
	return Eigen::Vector3f(config[0], config[1], config[2]);
}

int main(int argc, char* argv[]) {

	// *** Load the config file ***
	auto config = loadConfig("../config/config.json");

	const int pixHeight = config["pixHeight"], pixWidth = config["pixWidth"];
	const int nChannels = 4;

	// *** Set up camera and output image ***
	Camera cam(
		loadVec3FromConfig(config["cameraPos"]),
		loadVec3FromConfig(config["cameraForward"]),
		loadVec3FromConfig(config["cameraUp"]),
		pixWidth, pixHeight,
		config["cameraFov"]);


	std::vector<uint8_t> outImage(pixHeight * pixWidth * nChannels);

	Eigen::Vector3f
		red(1.f, 0.f, 0.f),
		blue(0.f, 0.f, 1.f),
		aqua(0.f, .8f, .8f),
		lavender(178.f / 255.f, 164.f / 255.f, 212.f / 255.f);

	Eigen::Vector3f
		room(0.461f, 0.441f, 0.375f),
		floor(0.833f, 0.791f, 0.647f),
		sofa(0.170f, 0.146f, 0.115f),
		bookshelf(0.160f, 0.118f, 0.084f),
		tvStand(1.0f, 0.903f, 0.734f),
		coffeeTable(0.098f, 0.054f, 0.022f),
		windowFrame(0.410f, 0.391f, 0.339f),
		shelf(0.325f, 0.378f, 0.311f),
		tv(0.009f, 0.007f, 0.005f);

	// *** Load shaders and textures ***
	std::vector<uint8_t> spotTexture;
	unsigned int width, height;
	lodepng::decode(spotTexture, width, height, "../models/spot.png");

	LambertianShader redLambertianShader(red);
	PhongShader bluePlasticShader(blue, Eigen::Vector3f(1.f, 1.f, 1.f), 100.f);
	LambertianShader aquaLambertianShader(aqua);
	LambertianShader lavenderLambertianShader(lavender);
	TexturedLambertianShader spotShader(&spotTexture, width, height);
	MirrorShader mirrorShader;
	TexCoordTestShader texCoordTestShader;

	LambertianShader
		roomLambertianShader(room),
		floorLambertianShader(floor),
		sofaLambertianShader(sofa),
		bookshelfLambertianShader(bookshelf),
		tvStandLambertianShader(tvStand),
		coffeeTableLambertianShader(coffeeTable),
		windowFrameLambertianShader(windowFrame),
		shelfLambertianShader(shelf);
	PhongShader tvPhongShader(tv, tv, 10.0f, false);

	// *** Set up scene ***
	Scene scene;

	// Optional code: here's how to add the spot mesh to the scene, using a BVH
	// Try enabling this and comparing it to the non-BVH version below!
	/*Model spotModel("../models/spot.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(spotModel, &spotShader, 4, rotateY(M_PI / 4.0f)));*/

	Model roomModel("../../../models/Room.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(roomModel, &roomLambertianShader, 4, rotateY(M_PI)));

	Model floorModel("../../../models/Floor.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(roomModel, &floorLambertianShader, 4, rotateY(M_PI)));

	Model bookshelfModel("../../../models/Bookshelf.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(bookshelfModel, &bookshelfLambertianShader, 4, rotateY(M_PI)));

	Model coffeeTableModel("../../../models/CoffeeTable.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(coffeeTableModel, &coffeeTableLambertianShader, 4, rotateY(M_PI)));

	Model shelfModel("../../../models/Shelf.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(shelfModel, &shelfLambertianShader, 4, rotateY(M_PI)));

	Model sofaModel("../../../models/Sofa.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(sofaModel, &sofaLambertianShader, 4, rotateY(M_PI)));

	Model tvModel("../../../models/TV.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(tvModel, &tvPhongShader, 4, rotateY(M_PI)));

	Model tvStandModel("../../../models/TVStand.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(tvStandModel, &tvStandLambertianShader, 4, rotateY(M_PI)));

	Model windowFrameModel("../../../models/WindowFrame.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(windowFrameModel, &windowFrameLambertianShader, 4, rotateY(M_PI)));

	Model windowPaneModel("../../../models/WindowPane.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(windowPaneModel, &mirrorShader, 4, rotateY(M_PI)));

	// Here's how to add the mesh without using the BVH.
	// Try comparing performance to the BVH version above.
	
	// [With BVN: 97.305 seconds]
	// [Without BVH: ~1700 seconds]

	/*Model spotModel("../models/spot.obj");
	scene.renderables.push_back(std::make_shared<Mesh>(&spotShader, &spotModel));
	scene.renderables.back()->modelToWorld(rotateY(M_PI / 4.0f));*/

	// *** Add lights to scene ***
	Eigen::Vector3f ambientLight(.1f, .1f, .1f);

	std::vector<std::unique_ptr<Light>> lightSources;
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(0.0f, 2.0f, 0.0f), 3.f * Eigen::Vector3f(1.f, 1.f, 1.f)));
	/*lightSources.push_back(std::make_unique<DirectionalLight>(Eigen::Vector3f(0.f, -1.f, 1.f), .5f * Eigen::Vector3f(1.f, 1.f, 1.f)));*/

	// *** Render the scene ***

	// Shuffling the scanline order gets better CPU usage between threads
	// when some lines take longer to render than others.
	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	if (config["shuffleScanlines"]) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(scanlines.begin(), scanlines.end(), g);
	}

	auto startTime = std::chrono::steady_clock::now();

	Ray ray = cam.getRay(531, 325);
	HitInfo hitInfo;
	scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK);
	float x = hitInfo.hitT;


	#pragma omp parallel for
	for (int y = 0; y < pixHeight; ++y) {
		for (int x = 0; x < pixWidth; ++x) {
			Ray ray = cam.getRay(x, scanlines[y]);
			HitInfo hitInfo;
			if (scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK)) {
				Eigen::Vector3f color = hitInfo.shader->getColor(
					hitInfo, &scene,
					lightSources, ambientLight,
					0, config["maxBounces"]);

				color.x() = std::min(color.x(), 1.f);
				color.y() = std::min(color.y(), 1.f);
				color.z() = std::min(color.z(), 1.f);


				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = color.x() * 255;
				outImage[(x + line * pixWidth) * nChannels + 1] = color.y() * 255;
				outImage[(x + line * pixWidth) * nChannels + 2] = color.z() * 255;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
			else {
				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = 0;
				outImage[(x + line * pixWidth) * nChannels + 1] = 0;
				outImage[(x + line * pixWidth) * nChannels + 2] = 0;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
		}
		if (omp_get_thread_num() == omp_get_num_threads()-1) {
			std::clog << "\rScanlines remaining: " << (pixHeight - y) << ' ' << std::flush;
		}

	}

	auto renderTime = std::chrono::steady_clock::now() - startTime;

	std::cout << "\nRender duration " << std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count() * 1e-3f << " seconds." << std::endl;

	// *** Save the output image ***
	int errorCode;
	errorCode = lodepng::encode(config["outputFilename"], outImage, pixWidth, pixHeight);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
