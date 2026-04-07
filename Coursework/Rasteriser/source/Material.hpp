#pragma once

#include <Eigen/Dense>
#include <algorithm>
#include "Shading.hpp"
#include "LinAlg.hpp"

class Material {
public:
	// Virtual destructor to prevent memory leaks
	virtual ~Material() = default;

	virtual Eigen::Vector3f shade(
		const Eigen::Vector3f normal,
		const Eigen::Vector3f viewDir,
		const Eigen::Vector3f lightDir,
		const Eigen::Vector3f lightIntensity
		) const = 0;	// Makes 'Material' an abstract base class meaning it can't be instantiated alone 
						// - forces derived classes to implement their own version of the function
};

// Using inheritance to inherit from base Material class
class Wood : public Material {
private:
	Eigen::Vector3f albedo;				// Colour of the material
	Eigen::Vector3f specularColor;		// The colour of the glossy reflection/highlight
	float specularExponent;				// Controls the "shininess" of the highlight

public:
	// Constructor using an initializer list to assign passed arguments to private class variables
	Wood(const Eigen::Vector3f& a, const Eigen::Vector3f& sC, float sE) 
		: albedo(a), specularColor(sC), specularExponent(sE) {}

	Eigen::Vector3f shade(
		const Eigen::Vector3f normal,
		const Eigen::Vector3f viewDir,
		const Eigen::Vector3f lightDir,
		const Eigen::Vector3f lightIntensity
	) const override {	// Overrides the shade function of the base class
		// Vectors must be normalised to avoid lighting distortion
		Eigen::Vector3f n = normal.normalized();

		// Checks if the light direction has zero length (for ambient light) and returns an unlit state
		if (lightDir.norm() <= 0.0f) {
			return coeffWiseMultiply<Eigen::Vector3f>(albedo, lightIntensity);
		}

		Eigen::Vector3f l = lightDir.normalized();
		Eigen::Vector3f v = viewDir.normalized();

		// Lambertian diffuse term
		// std::max(0.0f, ...) prevents negative light values - setting the minimum value to 0.0f
		float NdotL = std::max(0.0f, n.dot(l));
		Eigen::Vector3f diffuse = coeffWiseMultiply<Eigen::Vector3f>(albedo, lightIntensity) * NdotL;

		// Specular term
		float spec = blinnPhongSpecularTerm(l, n, v, specularExponent);
		Eigen::Vector3f specular = coeffWiseMultiply<Eigen::Vector3f>(specularColor, lightIntensity) * spec;

		return diffuse + specular;
	}
};

class Glass : public Material {
private:
	// No albedo because glass is mainly reflective
	Eigen::Vector3f specularColor;
	float specularExponent;

public:
	Glass(const Eigen::Vector3f& sC, float sE) : specularColor(sC), specularExponent(sE) {}

	Eigen::Vector3f shade(
		const Eigen::Vector3f normal,
		const Eigen::Vector3f viewDir,
		const Eigen::Vector3f lightDir,
		const Eigen::Vector3f lightIntensity
	) const override {
		// Normalise vectors
		Eigen::Vector3f n = normal.normalized();
		Eigen::Vector3f v = viewDir.normalized();

		// Ambient fallback
		if (lightDir.norm() <= 0.0f) {
			return coeffWiseMultiply<Eigen::Vector3f>(specularColor * 0.1f, lightIntensity);
		}

		Eigen::Vector3f l = lightDir.normalized();

		// Purely specular reflection
		float spec = blinnPhongSpecularTerm(l, n, v, specularExponent);
		return coeffWiseMultiply<Eigen::Vector3f>(specularColor, lightIntensity) * spec;
	}
};

class Fabric : public Material {
private:
	// Fabric material is matte/diffuse so it only requires base colour, no shininess (specular)
	Eigen::Vector3f albedo;

public:
	Fabric(const Eigen::Vector3f& a) : albedo(a) {}

	Eigen::Vector3f shade(
		const Eigen::Vector3f normal,
		const Eigen::Vector3f viewDir,
		const Eigen::Vector3f lightDir,
		const Eigen::Vector3f lightIntensity
	) const override {
		// Normalise vectors
		Eigen::Vector3f n = normal.normalized();

		// Ambient fallback
		if (lightDir.norm() <= 0.0f) {
			return coeffWiseMultiply<Eigen::Vector3f>(albedo, lightIntensity);
		}

		Eigen::Vector3f l = lightDir.normalized();

		// Prevent negative light
		float NdotL = std::max(0.0f, n.dot(l));
		return coeffWiseMultiply<Eigen::Vector3f>(albedo, lightIntensity) * NdotL;
	}
};
