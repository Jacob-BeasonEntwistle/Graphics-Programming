#pragma once
#include "Shader.hpp"
#include "GeomUtil.hpp"

/// <summary>
/// Glass-like shader that combines reflection and refraction.
/// Uses Fresnel reflectance and simple Phong-style lighting.
/// </summary>
class GlassShader : public Shader
{
private:
	float ior_;			// Index of Refraction controls the reflectance strength
	float shininess_;	// Specular exponent for Phong highlights

public:
	// Constructor initialises material properties
	GlassShader(float ior, float shininess) : ior_(ior), shininess_(shininess) {}

	// Calculates the final colour at a ray hit point using recursive reflection/refraction and direct lighting
	virtual Eigen::Vector3f getColor(const HitInfo& hitInfo,
		const Renderable* scene,
		const std::vector<std::unique_ptr<Light>>& lights,
		const Eigen::Vector3f& ambientLight,
		int currBounceCount,
		const int maxBounces) const
	{
		// Stop recursion once the maximum bounce depth is reached
		if (currBounceCount >= maxBounces) return Eigen::Vector3f::Zero();

		Ray reflectionRay;
		// Calculate the mirror reflection direction
		reflectionRay.direction = reflect(hitInfo.inDirection, hitInfo.normal);
		// Offset ray origin to avoid intersecting with itself
		reflectionRay.origin = hitInfo.location + 1e-4f * hitInfo.normal;

		float rO = (ior_ - 1) / (ior_ + 1);
		rO = rO * rO;
		// Cosine of the angle between the view direction and the surface normal
		float cosTheta = hitInfo.inDirection.dot(-hitInfo.normal);
		cosTheta = std::max(cosTheta, 0.0f);
		// Angle-dependent reflection coefficient
		float reflectRatio = rO + ((1 - rO) * powf(1 - cosTheta, 5));

		// Create a color variable to store the resulting color
		Eigen::Vector3f color = Eigen::Vector3f::Zero();

		HitInfo reflectionHit;
		if (scene->intersect(reflectionRay, 1e-6f, 1e4f, reflectionHit, VISIBLE_BITMASK)) {
			color = reflectionHit.shader->getColor(
				reflectionHit, scene,
				lights, ambientLight,
				currBounceCount + 1, maxBounces) * reflectRatio;
		}

		Ray refractiveRay;
		refractiveRay.direction = hitInfo.inDirection;
		// Offset ray slightly inside the surface
		refractiveRay.origin = hitInfo.location - 1e-4f * hitInfo.normal;

		HitInfo refractedHit;
		if (scene->intersect(refractiveRay, 1e-6f, 1e4f, refractedHit, VISIBLE_BITMASK)) {
			color = refractedHit.shader->getColor(
				refractedHit, scene,
				lights, ambientLight,
				currBounceCount + 1, maxBounces) * (1 - reflectRatio);
		}

		for (auto& light : lights) {
			if (!light->visibilityCheck(hitInfo.location, scene))
				continue;
			Eigen::Vector3f lightVec = light->getVecToLight(hitInfo.location);
			float dotProd = std::max(lightVec.dot(hitInfo.normal), 0.f);
			color += dotProd * light->getIntensity(hitInfo.location) * 0.2f;

			Eigen::Vector3f reflectVec = reflect(hitInfo.inDirection, hitInfo.normal);
			float dotSpec = std::max(lightVec.dot(reflectVec), 0.f);
			dotSpec = powf(dotSpec, shininess_);
			color += dotSpec * light->getIntensity(hitInfo.location) * 0.2f;
		}

		return color;
	}
};
