#pragma once
#include "Triangle.hpp"
#include "Shading.hpp"
#include "Light.hpp"
#include "Image.hpp"

void findScreenBoundingBox(const Triangle& t, int width, int height, int& minX, int& minY, int& maxX, int& maxY)
{
	// Find a bounding box around the triangle
	minX = std::min(std::min(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	minY = std::min(std::min(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());
	maxX = std::max(std::max(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	maxY = std::max(std::max(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());

	// Constrain it to lie within the image.
	minX = std::min(std::max(minX, 0), width - 1);
	maxX = std::min(std::max(maxX, 0), width - 1);
	minY = std::min(std::max(minY, 0), height - 1);
	maxY = std::min(std::max(maxY, 0), height - 1);
}

void drawTriangle(std::vector<uint8_t>& image, int width, int height,
	std::vector<float>& zBuffer,
	const Triangle& t,
	const std::vector<std::unique_ptr<Light>>& lights,
	const Eigen::Vector3f& albedo, const Eigen::Vector3f& specularColor,
	float specularExponent,
	const Eigen::Vector3f& camWorldPos)
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

	for (int x = minX; x <= maxX; ++x)
		for (int y = minY; y <= maxY; ++y) {
			Eigen::Vector2f p(x, y);

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

			float depth = t.screen[0].z() * b0 + t.screen[1].z() * b1 + t.screen[2].z() * b2;
			int depthIdx = static_cast<int>(p.x()) + static_cast<int>(p.y()) * width;
			if (depth > zBuffer[depthIdx]) continue;
			zBuffer[depthIdx] = depth;

			Eigen::Vector3f normP = t.norms[0] * b0 + t.norms[1] * b1 + t.norms[2] * b2;
			normP.normalize();

			// Work out colour at this position.
			Eigen::Vector3f color = Eigen::Vector3f::Zero();

			// Iterate over lights, and sum to find colour.
			for (auto& light : lights) {

				// Work out the contribution from this light source, and add it to the color variable.

				// Work out the intensity of this light source, at the point worldP.
				Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

				// We only need to do the following if the light isn't an ambient light.
				if (light->getType() != Light::Type::AMBIENT) {

					// Subtask 3: Work out correct inputs for the phongSpecularTerm function inside drawTriangle, and draw an image!
					// *** YOUR CODE HERE ***
					// Work out the incoming light dir (from the light into the surface point).
					Eigen::Vector3f incomingLightDir = light->getDirection(normP);
					// Work out the view direction (from surface point towards camera). Make sure it's normalized!
					Eigen::Vector3f viewDir = camWorldPos;
					// Find the specular term by calling phongSpecularTerm.
					float specularTerm = phongSpecularTerm(incomingLightDir, normP, viewDir, specularExponent);
					// *** END YOUR CODE ***

					Eigen::Vector3f specularOut = specularColor * specularTerm;
					specularOut = coeffWiseMultiply(specularOut, lightIntensity);

					// Take the dot product of the normal with the light direction.
					float dotProd = normP.dot(-incomingLightDir);

					// We don't want negative light - if dot product less than 0, set it to 0.
					dotProd = std::max(dotProd, 0.0f);

					// Multiply the light intensity by the dot product.
					Eigen::Vector3f diffuseOut = lightIntensity * dotProd;
					diffuseOut = coeffWiseMultiply(diffuseOut, albedo);

					// Add both diffuse and specular components to the colour.
					color += specularOut;
					color += diffuseOut;
				}
				else {
					// Light is ambient - just multiply light intensity with albedo.
					color += coeffWiseMultiply(lightIntensity, albedo);
				}
			}

			Color c;
			// Gamma-correcting colours.
			c.r = std::min(powf(color.x(), 1 / 2.2f), 1.0f) * 255;
			c.g = std::min(powf(color.y(), 1 / 2.2f), 1.0f) * 255;
			c.b = std::min(powf(color.z(), 1 / 2.2f), 1.0f) * 255;

			c.a = 255;

			setPixel(image, x, y, width, height, c);
		}
}
