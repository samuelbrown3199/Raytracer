#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>
#include <string>
#include <cstdlib>
#include <iostream>

#include "Common.h"

class ResourceTexture
{
public:

	ResourceTexture(const std::string& filename)  
	{
		Load(filename);
	}

	~ResourceTexture();

	bool Load(const std::string& filename);

	int Width()  const { return (fData == nullptr) ? 0 : imageWidth; }
	int Height() const { return (fData == nullptr) ? 0 : imageHeight; }

	const unsigned char* PixelData(int x, int y) const
	{
		// Return the address of the three RGB bytes of the pixel at x,y. If there is no image
		// data, returns magenta.
		static unsigned char magenta[] = { 255, 0, 255 };
		if (bData == nullptr) return magenta;

		x = Clamp(x, 0, imageWidth);
		y = Clamp(y, 0, imageHeight);

		return bData + y * bytesPerScanline + x * bytesPerPixel;
	}

private:

	const int      bytesPerPixel = 3;
	float* fData = nullptr;         // Linear floating point pixel data
	unsigned char* bData = nullptr;         // Linear 8-bit pixel data
	int            imageWidth = 0;         // Loaded image Width
	int            imageHeight = 0;        // Loaded image Height
	int            bytesPerScanline = 0;

	static int Clamp(int x, int low, int high) {
		// Return the value clamped to the range [low, high).
		if (x < low) return low;
		if (x < high) return x;
		return high - 1;
	}

	static unsigned char FloatToBytes(float value) {
		if (value <= 0.0)
			return 0;
		if (1.0 <= value)
			return 255;
		return static_cast<unsigned char>(256.0 * value);
	}

	void ConvertToBytes() {
		// Convert the linear floating point pixel data to bytes, storing the resulting byte
		// data in the `bdata` member.

		int total_bytes = imageWidth * imageHeight * bytesPerPixel;
		bData = new unsigned char[total_bytes];

		// Iterate through all pixel components, converting from [0.0, 1.0] float values to
		// unsigned [0, 255] byte values.

		auto* bptr = bData;
		auto* fptr = fData;
		for (auto i = 0; i < total_bytes; i++, fptr++, bptr++)
			*bptr = FloatToBytes(*fptr);
	}
};

class Texture
{
public:

	virtual ~Texture() = default;

	virtual Vector3 Value(double u, double v, const Vector3& p) const = 0;
};

class SolidColour : public Texture
{
public:

	SolidColour(const Vector3& albedo) : albedo(albedo) {}
	SolidColour(double red, double green, double blue) : albedo(red, green, blue) {}

	Vector3 Value(double u, double v, const Vector3& p) const override
	{
		return albedo;
	}

private:

	Vector3 albedo;
};

class CheckerTexture : public Texture
{
public:

	CheckerTexture(double scale, std::shared_ptr<Texture> even, std::shared_ptr<Texture> odd) : invScale(1.0 / scale), even(even), odd(odd) {}
	CheckerTexture(double scale, const Vector3& c1, const Vector3& c2)
		: invScale(1.0 / scale),
		even(std::make_shared<SolidColour>(c1)),
		odd(std::make_shared<SolidColour>(c2))
	{
	}

	Vector3 Value(double u, double v, const Vector3& p) const override
	{
		auto xInteger = int(std::float_denorm_style(invScale * p.x()));
		auto yInteger = int(std::float_denorm_style(invScale * p.y()));
		auto zInteger = int(std::float_denorm_style(invScale * p.z()));

		bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;
		return isEven ? even->Value(u, v, p) : odd->Value(u, v, p);
	}

private:
	double invScale;
	std::shared_ptr<Texture> odd;
	std::shared_ptr<Texture> even;
};

class ImageTexture : public Texture
{
public:

	ImageTexture(const std::string& filename) : m_texture(filename) {}

	Vector3 Value(double u, double v, const Vector3& p) const override
	{
		if (m_texture.Height() <= 0) return Vector3(0, 1, 1);

		// Clamp input texture coordinates to [0,1] x [1,0]
		u = Interval(0, 1).Clamp(u);
		v = 1.0 - Interval(0, 1).Clamp(v);  // Flip V to image coordinates

		auto i = int(u * m_texture.Width());
		auto j = int(v * m_texture.Height());
		auto pixel = m_texture.PixelData(i, j);

		auto color_scale = 1.0 / 255.0;
		return Vector3(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
	}

private:

	ResourceTexture m_texture;
};

#endif