#pragma once
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

// Chromatic aberration is a post-processing effect that simulates camera lens distortion by separating colour channels near the screen edge
class ChromaticAberration {
private:
	// Radius from the centre that should not be affected
	float unchangedRadius_;

	// Per channel distortion strength controlling how much each colour is distorted
	float slopeR_;
	float slopeG_;
	float slopeB_;

	// Calculates the radius beyond the unchanged radius
	float aberratedRadius(float radius, float slope, float unchangedRadius) {
		// Inside the unchanged radius, no distortion is applied
		if (radius < unchangedRadius) {
			return radius;
		}
		// Outside it, distort the radius more towards the image edges
		return slope * radius + unchangedRadius * (1 - slope);
	}

public:
	// Constructor - initialises distortion parameters for each channel
	ChromaticAberration(
		float unchangedRadius,
		float slopeR, float slopeG, float slopeB
	) : unchangedRadius_(unchangedRadius),
		slopeR_(slopeR), slopeG_(slopeG), slopeB_(slopeB) {
	}

	// Applies the chromatic abbertation to an output image
	std::vector<uint8_t> applyAberration(
		std::vector<uint8_t>& outImage,
		std::vector<uint8_t>& outImageChromatic,
		int pixWidth, int pixHeight,
		int nChannels) {
		// Iterate over every pixel in the image
		for (int y = 0; y < pixHeight; ++y) {
			for (int x = 0; x < pixWidth; ++x) {
				// Calculate the pixel offset from the centre of the image
				float xDist = x - pixWidth / 2;
				float yDist = y - pixHeight / 2;

				// Work out the distance from the centre
				float radius = sqrt(xDist * xDist + yDist * yDist);

				// Pixels near the centre remain unchanged
				if (radius < unchangedRadius_) {
					for (int c = 0; c < nChannels; c++) {
						outImageChromatic[(x + y * pixWidth) * nChannels + c] = outImage[(x + y * pixWidth) * nChannels + c];
					}
				}
				else {
					// Calculate separate radiuses for each channel
					float redRadius = aberratedRadius(radius, slopeR_, unchangedRadius_);
					float greenRadius = aberratedRadius(radius, slopeG_, unchangedRadius_);
					float blueRadius = aberratedRadius(radius, slopeB_, unchangedRadius_);

					// Normalise the direction from the image centre
					float xNorm = xDist / radius;
					float yNorm = yDist / radius;

					// Calculate the colour sample positions for each colour channel
					int rX = pixWidth / 2 + xNorm * redRadius;
					int rY = pixHeight / 2 + yNorm * redRadius;

					int gX = pixWidth / 2 + xNorm * greenRadius;
					int gY = pixHeight / 2 + yNorm * greenRadius;

					int bX = pixWidth / 2 + xNorm * blueRadius;
					int bY = pixHeight / 2 + yNorm * blueRadius;

					// Sample each colour channel from its displaced position
					outImageChromatic[(x + y * pixWidth) * nChannels + 0] = outImage[(rX + rY * pixWidth) * nChannels + 0];
					outImageChromatic[(x + y * pixWidth) * nChannels + 1] = outImage[(gX + gY * pixWidth) * nChannels + 1];
					outImageChromatic[(x + y * pixWidth) * nChannels + 2] = outImage[(bX + bY * pixWidth) * nChannels + 2];
					// Preserve the alpha channel to avoid unintended transparency
					outImageChromatic[(x + y * pixWidth) * nChannels + 3] = outImage[(x + y * pixWidth) * nChannels + 3];
				}
			}
		}
		// Return the final image with chromatic aberration applied
		return outImageChromatic;
	}
};
