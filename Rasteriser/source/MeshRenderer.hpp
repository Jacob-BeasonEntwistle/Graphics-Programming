#pragma once
#include <vector>
#include "Mesh.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/core/hal/interface.h>

extern cv::VideoWriter writer;

void drawMesh(std::vector<unsigned char>& image,
	std::vector<float>& zBuffer,
	const Mesh& mesh,
	Material* material,
	const Eigen::Vector3f& camWorldPos,
	const Eigen::Matrix4f& modelToWorld,
	const Eigen::Matrix4f& worldToClip,
	const std::vector<std::unique_ptr<Light>>& lights,
	int width, int height,
	const std::vector<uint8_t>& albedoTexture, int texWidth, int texHeight)
{
	cv::Mat lastFrame;
	int trianglesDrawn = 0;
	const int totalTriangles = mesh.vFaces.size();

	// Saving the rasterising animation
	cv::Mat prevImage;

	for (int i = 0; i < mesh.vFaces.size(); ++i) {
		Eigen::Vector3f
			v0 = mesh.verts[mesh.vFaces[i][0]],
			v1 = mesh.verts[mesh.vFaces[i][1]],
			v2 = mesh.verts[mesh.vFaces[i][2]];
		Eigen::Vector3f
			n0 = mesh.norms[mesh.nFaces[i][0]],
			n1 = mesh.norms[mesh.nFaces[i][1]],
			n2 = mesh.norms[mesh.nFaces[i][2]];

		Triangle t;
		t.verts[0] = (modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
		t.verts[1] = (modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
		t.verts[2] = (modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

		// Work out the clip space coordinates, by multiplying by worldToClip and doing the 
		// perspective divide.
		Eigen::Vector4f vClip0 = worldToClip * modelToWorld * vec3ToVec4(v0);
		vClip0 /= vClip0.w();
		Eigen::Vector4f vClip1 = worldToClip * modelToWorld * vec3ToVec4(v1);
		vClip1 /= vClip1.w();
		Eigen::Vector4f vClip2 = worldToClip * modelToWorld * vec3ToVec4(v2);
		vClip2 /= vClip2.w();

		// Check that all 3 vertices are in the clip box (-1 to 1 in x, y and z) and if not,
		// skip drawing this triangle.
		if (outsideClipBox(vClip0) || outsideClipBox(vClip1) || outsideClipBox(vClip2)) continue;

		// Work out the screen space coordinates based on the image height and width.
		t.screen[0] = Eigen::Vector3f((vClip0.x() + 1.0f) * width / 2, (-vClip0.y() + 1.0f) * height / 2, vClip0.z());
		t.screen[1] = Eigen::Vector3f((vClip1.x() + 1.0f) * width / 2, (-vClip1.y() + 1.0f) * height / 2, vClip1.z());
		t.screen[2] = Eigen::Vector3f((vClip2.x() + 1.0f) * width / 2, (-vClip2.y() + 1.0f) * height / 2, vClip2.z());

		// transform the normals (using the inverse transpose of the upper 3x3 block)
		t.norms[0] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n0).normalized();
		t.norms[1] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n1).normalized();
		t.norms[2] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n2).normalized();

		if (i < mesh.tFaces.size() && !mesh.texs.empty() &&
			mesh.tFaces[i][0] < mesh.texs.size() &&
			mesh.tFaces[i][1] < mesh.texs.size() &&
			mesh.tFaces[i][2] < mesh.texs.size()) 
		{
			t.texs[0] = mesh.texs[mesh.tFaces[i][0]];
			t.texs[1] = mesh.texs[mesh.tFaces[i][1]];
			t.texs[2] = mesh.texs[mesh.tFaces[i][2]];
		}
		else {
			// Generate fallback UVs if mesh doesn't have any
			t.texs[0] = Eigen::Vector2f(0.0f, 0.0f);
			t.texs[1] = Eigen::Vector2f(1.0f, 0.0f);
			t.texs[2] = Eigen::Vector2f(0.0f, 1.0f);
		}

		drawTriangle(image, width, height, zBuffer, t, lights, material, camWorldPos, albedoTexture, texWidth, texHeight);

		++trianglesDrawn;

		if (trianglesDrawn % 50 == 0) {
			cv::Mat currImage(cv::Size(width, height), CV_8UC4, image.data());
			cv::Mat currImageBGR;
			cv::cvtColor(currImage, currImageBGR, cv::COLOR_RGBA2BGR);
			
			float percent = (float(trianglesDrawn) / float(totalTriangles)) * 100.0f;

			std::string text =
				"Rasterising: " + std::to_string(trianglesDrawn) +
				" / " + std::to_string(totalTriangles) +
				" (" + std::to_string(int(percent)) + "%)";

			cv::putText(
				currImageBGR,
				text,
				cv::Point(22, height - 52),
				cv::FONT_HERSHEY_SIMPLEX,
				1.0,
				cv::Scalar(0, 0, 0),
				2
			);

			cv::putText(
				currImageBGR,
				text,
				cv::Point(20, height - 50),
				cv::FONT_HERSHEY_SIMPLEX,
				1.0,
				cv::Scalar(255, 255, 255),
				2
			);

			if (prevImage.empty()) {
				// This is the first triangle drawn, so definitely save the image
				writer.write(currImageBGR);
				currImageBGR.copyTo(prevImage);
			}
			else {
				// For subsequent images, check if the image is different to the previous one
				cv::Scalar diff = cv::sum(currImageBGR - prevImage);
				if (diff[0] != 0 || diff[1] != 0 || diff[2] != 0) {
					// Image is different, so save it.
					writer.write(currImageBGR);
					currImageBGR.copyTo(prevImage);
				}
			}

			lastFrame = currImageBGR.clone();
		}
	}
}
