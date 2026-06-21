#pragma once
#include "Shader.hpp"

/// <summary>
/// Shader using the classic Phong reflectance model to add specular highlights.
/// </summary>
class PhongShader : public Shader
{
private:
	Eigen::Vector3f albedo_, specular_;
	float shininess_;
	bool shadowTest_;
public:
	PhongShader(const Eigen::Vector3f& albedo, const Eigen::Vector3f& specular, float shininess, bool shadowTest=true)
		:albedo_(albedo), specular_(specular), shininess_(shininess), shadowTest_(shadowTest)
	{}

	virtual Eigen::Vector3f getColor(const HitInfo& hitInfo, 
		const Renderable* scene, 
		const std::vector<std::unique_ptr<Light>>& lights,
		const Eigen::Vector3f& ambientLight,
		int currBounceCount,
		const int maxBounces) const
	{
		Eigen::Vector3f color = coefftWiseMul(albedo_, ambientLight);

		for (auto& light : lights) {
			if (shadowTest_) {
				if (!light->visibilityCheck(hitInfo.location, scene))
					continue;
			}
			Eigen::Vector3f lightVec = light->getVecToLight(hitInfo.location);
			float dotProd = std::max(lightVec.dot(hitInfo.normal), 0.f);
			color += dotProd * coefftWiseMul(light->getIntensity(hitInfo.location), albedo_);

			Eigen::Vector3f reflectVec = reflect(hitInfo.inDirection, hitInfo.normal);
			float dotSpec = std::max(lightVec.dot(reflectVec), 0.f); 
			dotSpec = powf(dotSpec, shininess_);
			color += dotSpec * coefftWiseMul(light->getIntensity(hitInfo.location), specular_);
		}

		return color;
	}
};

