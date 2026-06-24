#pragma once

#include "Triangle.hpp"
#include "Shading.hpp"
#include "Light.hpp"
#include "Image.hpp"
#include "Material.hpp"

// Calculates 2D screen bounding box limits for a triangle - optimises rendering performance
void findScreenBoundingBox(const Triangle& t, int width, int height, int& minX, int& minY, int& maxX, int& maxY)
{
	// Find a bounding box around the triangle
	minX = std::min(std::min(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	minY = std::min(std::min(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());
	maxX = std::max(std::max(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	maxY = std::max(std::max(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());

	// Constrain it to lie within the image.
	minX = (int)std::floor(minX);
	maxX = (int)std::ceil(maxX);
	minY = (int)std::floor(minY);
	maxY = (int)std::ceil(maxY);
}

void drawTriangle(std::vector<uint8_t>& image, int width, int height,
	std::vector<float>& zBuffer,
	const Triangle& t,
	const std::vector<std::unique_ptr<Light>>& lights,
	Material* material,		// Takes Material pointer to allow polymorphic shading - different logic for each material
	const Eigen::Vector3f& camWorldPos,
	const std::vector<uint8_t>& albedoTexture, int texWidth, int texHeight)
{
	int minX, minY, maxX, maxY;
	findScreenBoundingBox(t, width, height, minX, minY, maxX, maxY);

	Eigen::Vector2f edge1 = v2(t.screen[2] - t.screen[0]);
	Eigen::Vector2f edge2 = v2(t.screen[1] - t.screen[0]);
	float triangleArea = 0.5f * vec2Cross(edge2, edge1);
	if (triangleArea < 0) {
		// Triangle is backfacing
		// Exit and quit drawing!
		return;
	}

	// Create 4 pre-defined sub-pixel sample locations (2x2 SSAA grid)
	std::vector<Eigen::Vector2f> sampleOffsets;
	sampleOffsets.push_back(Eigen::Vector2f(0.25f, 0.25f));
	sampleOffsets.push_back(Eigen::Vector2f(0.75f, 0.25f));
	sampleOffsets.push_back(Eigen::Vector2f(0.25f, 0.75f));
	sampleOffsets.push_back(Eigen::Vector2f(0.75f, 0.75f));

	for (int x = minX; x <= maxX; ++x)
		for (int y = minY; y <= maxY; ++y) {
			// Supersampling Anti-Aliasing (SSAA) variables
			Eigen::Vector3f totalColor(0.0f, 0.0f, 0.0f);	// Create a total colour variable
			// Set the initial closest depth to the highest value possible
			float closestDepth = std::numeric_limits<float>::infinity();
			int validSamples = 0;

			// For each sample
			for (int s = 0; s < sampleOffsets.size(); s++) {
				// Work out the point
				Eigen::Vector2f p(x + sampleOffsets[s].x(), y + sampleOffsets[s].y());

				// Find sub-triangle areas
				float a0 = 0.5f * fabsf(vec2Cross(v2(t.screen[1]) - v2(t.screen[2]), p - v2(t.screen[2])));
				float a1 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[2]), p - v2(t.screen[2])));
				float a2 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[1]), p - v2(t.screen[1])));

				// find barycentrics
				float b0 = a0 / triangleArea;
				float b1 = a1 / triangleArea;
				float b2 = a2 / triangleArea;

				// If outside triangle, exit early
				float sum = b0 + b1 + b2;
				if (sum > 1.0001) {
					continue;
				}

				Eigen::Vector3f worldP = t.verts[0] * b0 + t.verts[1] * b1 + t.verts[2] * b2;

				// Perform depth testing (Z-buffering) to ensure only the closest triangle is drawn
				// Depth per sample
				float depth = t.screen[0].z() * b0 + t.screen[1].z() * b1 + t.screen[2].z() * b2;
				
				if (depth < closestDepth) {
					closestDepth = depth;
				}

				Eigen::Vector3f normP = t.norms[0] * b0 + t.norms[1] * b1 + t.norms[2] * b2;
				normP.normalize();

				// Calculate the texture coordinates corresponding to P, texP using barycentric interpolation
				Eigen::Vector2f texP = t.texs[0] * b0 + t.texs[1] * b1 + t.texs[2] * b2;

				// Convert the coordinate to a point in texture space
				int texR = (1.0f - texP.y()) * (texHeight - 1);		// Row index
				int texC = texP.x() * (texWidth - 1);				// Column index

				// Clamp texR and texC to the image bounds
				if (texR >= texHeight) texR = texHeight - 1;
				if (texR < 0) texR = 0;
				if (texC >= texWidth) texC = texWidth - 1;
				if (texC < 0) texC = 0;

				// Get the value from the texture using the getPixel function on the albedoTexture
				Color texColor{ getPixel(albedoTexture, texC, texR, texWidth, texHeight) };

				// Normalise the values
				float linR = texColor.r / 255.0f;
				float linG = texColor.g / 255.0f;
				float linB = texColor.b / 255.0f;

				// Use 2.2 to convert from the display space (gamma-encoded) into linear space (correct light physics)
				float r = pow(linR, 2.2);
				float g = pow(linG, 2.2);
				float b = pow(linB, 2.2);

				// Convert it into an Eigen::Vector3f as an albedo
				Eigen::Vector3f albedo = Eigen::Vector3f(r, g, b);

				// Work out colour at this position.
				Eigen::Vector3f color = Eigen::Vector3f::Zero();

				// Iterate over lights, and sum to find colour.
				for (auto& light : lights) {

					// Work out the contribution from this light source, and add it to the color variable.

					// Work out the intensity of this light source, at the point worldP.
					Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

					// We only need to do the following if the light isn't an ambient light.
					if (light->getType() != Light::Type::AMBIENT) {

						Eigen::Vector3f lightPos = light->getLightLocation();
						// Work out the incoming light dir (from the light into the surface point).
						Eigen::Vector3f incomingLightDir = (lightPos - worldP).normalized();
						// Work out the view direction (from surface point towards camera). Make sure it's normalized!
						Eigen::Vector3f viewDir = (camWorldPos - worldP).normalized();

						// Dynamically calculating the light contribution using the materials specific virtual shade function
						Eigen::Vector3f contribution = material->shade(normP, viewDir, incomingLightDir, lightIntensity);
						// Apply the texture (albedo) to the light contribution
						color += contribution.cwiseProduct(albedo);
					}
					// If the light is ambient light there is no light direction
					else {
						Eigen::Vector3f viewDir = (camWorldPos - worldP).normalized();
						Eigen::Vector3f contribution = material->shade(normP, viewDir, Eigen::Vector3f::Zero(), lightIntensity);
						color += contribution.cwiseProduct(albedo);
					}
				}

				// Add the colour of the sample calculated to the total colour
				totalColor += color;
				// Increment the number of valid samples
				validSamples++;
			}

			// If there are no valid samples, return nothing (black)
			if (validSamples == 0) continue;

			// Divide the total colour by the number of samples to get an average
			Eigen::Vector3f finalColor = totalColor / validSamples;

			if (x < 0 || x >= width || y < 0 || y >= height) {
				continue;
			}

			int depthIdx = x + y * width;

			if (closestDepth > zBuffer[depthIdx]) continue;
			zBuffer[depthIdx] = closestDepth;

			Color c;
			// Gamma-correcting colours.
			c.r = std::min(powf(finalColor.x(), 1 / 2.2f), 1.0f) * 255;
			c.g = std::min(powf(finalColor.y(), 1 / 2.2f), 1.0f) * 255;
			c.b = std::min(powf(finalColor.z(), 1 / 2.2f), 1.0f) * 255;

			c.a = 255;

			int flippedX = width - x - 1;
			if (flippedX < 0 || flippedX >= width) continue;

			// Flip the x value of the pixel being drawn to draw the scene flipped horizontally
			setPixel(image, flippedX, y, width, height, c);
		}
}
