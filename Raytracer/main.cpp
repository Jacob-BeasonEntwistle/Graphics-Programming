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
#include "TexturedPhongShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "GlassShader.hpp"
#include "Model.hpp"
#include "ChromaticAberration.hpp"
#include <fstream>
#include <iomanip>

#include <opencv2/opencv.hpp>
#include <opencv2/core/hal/interface.h>

cv::VideoWriter writer;

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

//// Returns a random float value between 0 and 1
//// Used for the jitter of Anti-Aliasing
//float randomFloat() {
//	return static_cast<float>(rand()) / RAND_MAX * 1.0f;
//}

float randomFloat_thread() {
	thread_local std::mt19937 gen(std::random_device{}());
	thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	return dist(gen);
}

int main(int argc, char* argv[]) {

	// --[Load the config file]--
	auto config = loadConfig("../config/config.json");

	// ::DEBUGGING:: Could divide pixHeight & pixWidth by 2 to reduce resolution = faster render time
	const int pixHeight = config["pixHeight"], pixWidth = config["pixWidth"];
	const int nChannels = 4;

	// --[Set up camera and output image]--
	Camera cam(
		loadVec3FromConfig(config["cameraPos"]),
		loadVec3FromConfig(config["cameraForward"]),
		loadVec3FromConfig(config["cameraUp"]),
		pixWidth, pixHeight,
		config["cameraFov"]);


	std::vector<uint8_t> outImage(pixHeight * pixWidth * nChannels);
	std::vector<uint8_t> outImageChromatic(pixHeight * pixWidth * nChannels);

	// --[Scene Colours]--
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

	// --[Load shaders and textures]--
	// Create a vector of paths for the textures to be loaded
	std::vector<std::string> texturePaths;
	texturePaths.push_back("../../assets/textures/Leather030_Color.png");	// [0]
	texturePaths.push_back("../../assets/textures/Carpet016_Color.png");	// [1]
	texturePaths.push_back("../../assets/textures/Plaster001_Color.png");	// [2]
	texturePaths.push_back("../../assets/textures/Wood066_Color.png");		// [3]
	texturePaths.push_back("../../assets/textures/Wood095_Color.png");		// [4]
	texturePaths.push_back("../../assets/textures/Plastic011_Color.png");	// [5]

	// Create vectors of the textures, widths, and heights to store the values for later use
	std::vector<std::vector<uint8_t>> textures;
	std::vector<unsigned int> widths, heights;

	// For each path in the vector of paths create a texture, width, and height variable and decode the png into those values
	for (std::string path : texturePaths) {
		std::vector<uint8_t> texture;
		unsigned int width, height;
		unsigned int error = lodepng::decode(texture, width, height, path);

		// Check if lodepng returned null
		if (error != 0) {
			std::cerr << "Error: Failed to load texture " << path << " : " << lodepng_error_text(error) << std::endl;
			return 1;
		}

		// Checks if the texture is empty or the width/height are 0
		if (texture.empty() || width == 0 || height == 0) {
			std::cerr << "Error: Invalid texture data" << std::endl;
			return 1;
		}

		// If no errors occur, store the textures (avoids storing empty textures)
		textures.push_back(texture);
		widths.push_back(width);
		heights.push_back(height);
	}

	// --[Scene Shaders]--
	TexturedLambertianShader
		leatherTexLambertianShader(&textures[0], widths[0], heights[0]),
		carpetTexLambertianShader(&textures[1], widths[1], heights[1]),
		plasterTexLambertianShader(&textures[2], widths[2], heights[2]);
	TexturedPhongShader
		DarkWoodTexPhongShader(&textures[3], widths[3], heights[3], Eigen::Vector3f(0.2f, 0.2f, 0.2f), 50.0f),
		LightWoodTexPhongShader(&textures[4], widths[4], heights[4], Eigen::Vector3f(0.1f, 0.1f, 0.1f), 30.0f),
		plasticPhongShader(&textures[5], widths[5], heights[5], Eigen::Vector3f(0.6f, 0.6f, 0.6f), 70.0f);
	MirrorShader mirrorShader;
	GlassShader glassShader(1.4f, 50.0f);

	// --[Set up scene]--
	Scene scene;

	// [With BVN: 97.305 seconds]
	// [Without BVH: ~1700 seconds]

	// TEST 1: Adding the spot mesh to the scene using a BVH
	/*Model spotModel("../models/spot.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(spotModel, &spotShader, 4, rotateY(M_PI / 4.0f)));*/

	// TEST 2: Adding the mesh without using the BVH.
	/*Model spotModel("../models/spot.obj");
	scene.renderables.push_back(std::make_shared<Mesh>(&spotShader, &spotModel));
	scene.renderables.back()->modelToWorld(rotateY(M_PI / 4.0f));*/

	Model roomModel("../../assets/models/Room_complete.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(roomModel, &plasterTexLambertianShader, 4, rotateY(M_PI)));

	Model bookshelfModel("../../assets/models/Bookshelf.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(bookshelfModel, &DarkWoodTexPhongShader, 4, rotateY(M_PI)));

	Model coffeeTableModel("../../assets/models/CoffeeTable.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(coffeeTableModel, &DarkWoodTexPhongShader, 4, rotateY(M_PI)));

	Model shelfModel("../../assets/models/Shelf.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(shelfModel, &LightWoodTexPhongShader, 4, rotateY(M_PI)));

	Model sofaModel("../../assets/models/Sofa.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(sofaModel, &leatherTexLambertianShader, 4, rotateY(M_PI)));

	Model tvModel("../../assets/models/TV.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(tvModel, &plasticPhongShader, 4, rotateY(M_PI)));

	Model tvStandModel("../../assets/models/TVStand.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(tvStandModel, &LightWoodTexPhongShader, 4, rotateY(M_PI)));

	Model windowFrameModel("../../assets/models/WindowFrame.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(windowFrameModel, &LightWoodTexPhongShader, 4, rotateY(M_PI)));

	Model windowPaneModel("../../assets/models/WindowPane.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(windowPaneModel, &glassShader, 4, rotateY(M_PI)));

	// --[Add lights to scene]--
	Eigen::Vector3f ambientLight(0.02f, 0.02f, 0.02f);

	std::vector<std::unique_ptr<Light>> lightSources;
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(0.0f, 2.0f, 0.0f), 3.0f * Eigen::Vector3f(1.f, 1.f, 1.f)));

	// --[Render the scene]--
	// Shuffling the scanline order gets better CPU usage between threads
	// when some lines take longer to render than others.
	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	/*if (config["shuffleScanlines"]) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(scanlines.begin(), scanlines.end(), g);
	}*/

	auto startTime = std::chrono::steady_clock::now();

	// Accumulation buffer (for animation)
	std::vector<Eigen::Vector3f> accumulation(pixWidth* pixHeight, Eigen::Vector3f::Zero());

	writer = cv::VideoWriter("raytrace.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 30, cv::Size(pixWidth, pixHeight));

	if (!writer.isOpened()) {
		std::cout << "ERROR: VideoWriter failed!" << std::endl;
		return -1;
	}

	cv::Mat lastFrame;

	std::atomic<int> completedLines(0);

	// Anti-Aliasing using stochastic supersampling (SSAA)
	// Reducing the number of samples reduces the affect of the AA but increases the render time
	// Increasing the num of samples increases the affect of AA but increases render time
	int samplesPerPixel = 24;

	for (int sample = 0; sample < samplesPerPixel; sample++) {
		completedLines = 0;

	// Threading disabled to allow random function of AA to work as intended - creating multiple rays with random offsets
	#pragma omp parallel for
		for (int y = 0; y < pixHeight; ++y) {
			for (int x = 0; x < pixWidth; ++x) {
				// Anti-Aliasing implemented using stochastic (random) supersampling
								
				// Generate a random jittered pixel offsets (stochastic SSAA)
				float jitterX = x + randomFloat_thread() - 0.5f;
				//float jitterY = scanlines[y] + randomFloat() - 0.5f;
				float jitterY = y + randomFloat_thread() - 0.5f;

				// ::DEBUGGING:: to test whether the jitter values are changing
				/*if (x == pixWidth / 2 && y == pixHeight / 2 && s < 5) {
					std::cout << jitterX << ", " << jitterY << std::endl;
				}*/

				// Shoot a ray using that value
				Ray ray = cam.getRay(jitterX, jitterY);
				HitInfo hitInfo;
				hitInfo.shader = nullptr;

				// If the ray hits an object in the scene
				if (scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK)) {
					if (hitInfo.shader == nullptr) {
						std::cerr << "Error: Shader is null!" << std::endl;
						continue;
					}

					// Get the colour of that point
					Eigen::Vector3f color = hitInfo.shader->getColor(
						hitInfo, &scene,
						lightSources, ambientLight,
						0, config["maxBounces"]);

					// And add it to the total colour
					accumulation[x + y * pixWidth] += color;
				}
				else {
					// If ray misses an object in the scene, it returns black
					Eigen::Vector3f color(0.0f, 0.0f, 0.0f);
					// Black is still added to the total colour
					accumulation[x + y * pixWidth] += color;
				}

			}

			int done = completedLines.fetch_add(1) + 1;

			float percent = (done / float(pixHeight)) * 100.0f;

			if (done % 20 == 0) {
				std::cout << "\r[Sample " << sample + 1
					<< " / " << samplesPerPixel
					<< "] " << std::fixed << std::setprecision(1) << percent << "%"
					<< "  " << std::flush;
			}
			if (done == pixHeight) {
				std::cout << "\r[Sample " << sample + 1
					<< " / " << samplesPerPixel
					<< "] " << "100.0%"
					<< "  " << std::flush;
			}
		}

		std::cout << std::endl;

		for (int y = 0; y < pixHeight; y++) {
			for (int x = 0; x < pixWidth; x++) {
				Eigen::Vector3f col = accumulation[x + y * pixWidth] / float(sample + 1);

				if (sample < 5) {
					float factor = 0.4f + 0.15f * sample;
					col *= factor;	// Darker -> looks noisier
				}

				if (sample < 3) {
					float noise = randomFloat_thread() * 0.12f;
					col += col.cwiseProduct(Eigen::Vector3f(noise, noise, noise));
				}
				
				if (sample > 4) {
					col = col.cwiseMin(1.0f);
				}

				// Gamma correction
				if (sample < 4) {
					col = col.array().pow(1.0f / 1.6f);
				}
				else {
					col = col.array().pow(1.0f / 2.2f);
				}

				int flippedY = pixHeight - 1 - y;
				int idx = (x + flippedY * pixWidth) * 4;

				outImage[idx + 0] = std::min(col.x(), 1.0f) * 255;
				outImage[idx + 1] = std::min(col.y(), 1.0f) * 255;
				outImage[idx + 2] = std::min(col.z(), 1.0f) * 255;
				outImage[idx + 3] = 255;
			}
		}

		cv::Mat frame(pixHeight, pixWidth, CV_8UC4, outImage.data());
		cv::Mat bgr;
		cv::cvtColor(frame, bgr, cv::COLOR_RGBA2BGR);

		float progress = float(sample + 1) / samplesPerPixel * 100.0f;

		std::string text =
			"Sample " + std::to_string(sample + 1) +
			" / " + std::to_string(samplesPerPixel) +
			" (" + std::to_string(int(progress)) + "%)";

		cv::putText(
			bgr,
			text,
			cv::Point(22, pixHeight - 52),
			cv::FONT_HERSHEY_SIMPLEX,
			1.0,
			cv::Scalar(0, 0, 0),
			2
		);

		cv::putText(
			bgr,
			text,
			cv::Point(20, pixHeight - 50),
			cv::FONT_HERSHEY_SIMPLEX,
			1.0,
			cv::Scalar(255, 255, 255),
			2
		);

		int repeats = (sample < 5) ? 8 : 2;
		for (int i = 0; i < repeats; i++) {
			writer.write(bgr);
		}

		lastFrame = bgr.clone();
	}

	// Hold final frame for ~2 seconds
	for (int i = 0; i < 60; i++) {  // 60 frames @ 30fps = 2 seconds
		writer.write(lastFrame);
	}

	// Initialise the chromatic aberration with the unchangedRadius, and different slopes for each RGB channel
	ChromaticAberration ca(500.0f, 1.0f, 0.95f, 0.9f);
	// Apply the post-processing chromatic aberration to simulate the required lens distortion
	auto result = ca.applyAberration(outImage, outImageChromatic, pixWidth, pixHeight, nChannels);

	auto renderTime = std::chrono::steady_clock::now() - startTime;

	std::cout << "\nRender duration " << std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count() * 1e-3f << " seconds." << std::endl;

	// --[Save the output image]--
	int errorCode;
	errorCode = lodepng::encode(config["outputFilename"], outImage, pixWidth, pixHeight);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}
	errorCode = lodepng::encode("output_aberrated.png", outImageChromatic, pixWidth, pixHeight);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
