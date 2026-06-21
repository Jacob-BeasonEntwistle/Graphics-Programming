#pragma once
#include <Eigen/Dense>

/// <summary>
/// Reflect an incoming vector in a normal.
/// </summary>
/// <param name="incoming">Incoming direction unit vector, pointing into surface.</param>
/// <param name="normal">Surface normal</param>
/// <returns>Reflected vector, pointing out from surface point.</returns>
Eigen::Vector3f reflect(const Eigen::Vector3f& incoming, const Eigen::Vector3f& normal)
{
	Eigen::Vector3f i = incoming.normalized();
	Eigen::Vector3f n = normal.normalized();

	Eigen::Vector3f reflection;
	reflection = i - 2 * i.dot(n) * n;

	return reflection;
}

/// <summary>
/// Return the Phong specular term for a given lighting direction, normal, view direction and specular exponent.
/// NOTE: All input vectors must be normalised unit vectors.
/// </summary>
/// <param name="incomingLightDir">Unit direction vector from light towards surface point.</param>
/// <param name="normal">Normal at surface point.</param>
/// <param name="viewDir">Direction unit vector from surface point towards viewing camera.</param>
/// <param name="exponent">Specular exponent (higher=shinier)</param>
/// <returns>Specular term (number from 0 to 1)</returns>
float phongSpecularTerm(const Eigen::Vector3f& incomingLightDir, const Eigen::Vector3f& normal, const Eigen::Vector3f& viewDir, float exponent)
{
	// Find the reflected direction using the reflect function
	Eigen::Vector3f reflectionDir = reflect(incomingLightDir, normal);

	// Find dot product between reflected and view directions.
	float reflectDotNorm = reflectionDir.dot(viewDir);

	// Make sure dot product is non-negative (if it's less than 0, set it to 0!)
	if (reflectDotNorm <= 0) {
		reflectDotNorm = 0;
	}

	// Finally, raise to specular exponent and return.
	return pow(reflectDotNorm, exponent);
}

/// <summary>
/// Return the Blinn-Phong specular term for a given lighting direction, normal, view direction and specular exponent.
/// NOTE: All input vectors must be normalised unit vectors.
/// </summary>
/// <param name="incomingLightDir">Unit direction vector from light towards surface point.</param>
/// <param name="normal">Normal at surface point.</param>
/// <param name="viewDir">Direction unit vector from surface point towards viewing camera.</param>
/// <param name="exponent">Specular exponent (higher=shinier)</param>
/// <returns>Specular term (number from 0 to 1)</returns>
float blinnPhongSpecularTerm(const Eigen::Vector3f& incomingLightDir, const Eigen::Vector3f& normal, const Eigen::Vector3f& viewDir, float exponent)
{
	// Find the half-vector (average of view dir and light dir)
	Eigen::Vector3f halfVec = (incomingLightDir + viewDir).normalized();

	// Find dot product of half-vector and normal.
	float halfDotNorm = normal.dot(halfVec);

	// Force the dot product to be non-negative (if <0, set to 0)
	if (halfDotNorm < 0) {
		halfDotNorm = 0;
	}

	//Return the dot product raised to the exponent
	return pow(halfDotNorm, exponent);
}
