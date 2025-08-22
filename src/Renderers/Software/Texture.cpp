#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

ResourceTexture::~ResourceTexture()
{
	delete[] bData;
	STBI_FREE(fData);
}

bool ResourceTexture::Load(const std::string& filename) {
	// Loads the linear (gamma=1) image data from the given file name. Returns true if the
	// load succeeded. The resulting data buffer contains the three [0.0, 1.0]
	// floating-point values for the first pixel (red, then green, then blue). Pixels are
	// contiguous, going left to right for the Width of the image, followed by the next row
	// below, for the full Height of the image.

	auto n = bytesPerPixel; // Dummy out parameter: original components per pixel
	fData = stbi_loadf(filename.c_str(), &imageWidth, &imageHeight, &n, bytesPerPixel);
	if (fData == nullptr) return false;

	bytesPerScanline = imageWidth * bytesPerPixel;
	ConvertToBytes();
	return true;
}