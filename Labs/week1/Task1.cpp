#include <iostream>
#include <lodepng.h>
#include <cmath>

const int width = 1920, height = 1080;
const int nChannels = 4;

// Functions can't be called within one another so it has to be declared outside of main()
// Doing this means it can't access variables local to main() so the variables also have to be moved outside

// [TASK 2: Answer]
// This is a function to handle setting a pixels RGBA values
// Using a pointer to the uint8_t is okay but passing it by reference would be better
void setPixel(uint8_t* _imageBuffer, int _x, int _y, int _r, int _g, int _b, int _a) {
	int pixelIdx = _x + _y * width;
	_imageBuffer[pixelIdx * nChannels + 0] = _r;
	_imageBuffer[pixelIdx * nChannels + 1] = _g;
	_imageBuffer[pixelIdx * nChannels + 2] = _b;
	_imageBuffer[pixelIdx * nChannels + 3] = _a;
}

int main()
{
	std::string outputFilename = "output.png";

	// Setting up an image buffer
	// This std::vector has one 8-bit value for each pixel in each row and column of the image, and
	// for each of the 4 channels (red, green, blue and alpha).
	// Remember 8-bit unsigned values can range from 0 to 255.
	std::vector<uint8_t> imageBuffer(height*width*nChannels);

	// [TASK 1: Answer]
	// One way is starting y at height / 2 BUT could have also used an if statement
	// This for loop sets all the pixels of the image to a cyan (green - Task 1) colour.
	/*for (int y = height / 2; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int pixelIdx = x + y * width;
			imageBuffer[pixelIdx * nChannels + 0] = 0; // Set red pixel values to 0
			imageBuffer[pixelIdx * nChannels + 1] = 255; // Set green pixel values to 255 (full brightness)
			imageBuffer[pixelIdx * nChannels + 2] = 0; // Set blue pixel values to 255 (full brightness)
			imageBuffer[pixelIdx * nChannels + 3] = 255; // Set alpha (transparency) pixel values to 255 (fully opaque)
		}
	}*/

	// [TASK 2: Answer]
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (y < height / 2) {
				// .data() gets the first index of the vector
				setPixel(imageBuffer.data(), x, y, 255, 0, 0, 255);
			}
			else {
				setPixel(imageBuffer.data(), x, y, 0, 255, 0, 255);
			}
		}
	}

	// [TASK 3: Answer]
	int radius = 500;
	float centerX = width * 0.5f;
	float centerY = height * 0.5f;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			// Previously used sqrt() and ^ 2 thinking that would get the square root of the square
			// ^ 2 doesn't work the way I assumed, ^ means XOR not squared by
			if ((((x - centerX) * (x - centerX)) + ((y - centerY) * (y - centerY))) < radius) {
				setPixel(imageBuffer.data(), x, y, 0, 0, 0, 255);
			}
		}
	}

	// *** Encoding image data ***
	// PNG files are compressed to save storage space. 
	// The lodepng::encode function applies this compression to the image buffer and saves the result 
	// to the filename given.
	int errorCode;
	errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}

/// *** Lab Tasks ***
// * (DONE) Task 1: Try adapting the code above to set the lower half of the image to be a green colour.

// * (DONE) Task 2: Doing the maths above to work out indices is a bit annoying! Write your own setPixel function.
//           - This should take x and y coordinates as input, and red, green, blue and alpha values.
//           - Remember to pass in your imageBuffer. Should it be passed in by reference or by value? Should
//           the reference be const?
//           (We will use this setPixel function to build our rasteriser in the upcoming labs.)
//			 - Test your setPixel function by setting pixels in your image to different colours.

// * (DONE) Task 3: Use your setPixel function to draw a circle in the centre of the image. Remember a point is
//           in a circle if sqrt((x - x_0)^2 + (y - y_0)^2) < radius (here x_0, y_0 are the coordinates at the middle of 
//           the circle). 
//           Hint - use a similar for loop to the one above, and add an if statement to check if the current
//           pixel lies in the circle.
//           Try modifying the order you draw each component in. If you draw the circle before setting the lower 
//           part of the image to be green, how does this modify the image?

// * (Optional) Task 4: Work out how good the compression ratio of the saved PNG image is. PNG images
//           use *lossless* compression, where all the pixel values of the original image are preserved.
//           To work out the compression ratio, compare the size of the saved image to the memory
//           occupied by the image buffer (this is based on the width, height and number of channels).
//           Try setting the pixels to random values (use rand() and the % operator). What is the 
//           compression ratio now, and why do you think this is?
