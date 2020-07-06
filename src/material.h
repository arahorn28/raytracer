#ifndef RAYTRACER_MATERIAL_H_
#define RAYTRACER_MATERIAL_H_

#include "color.h"
#include "lodepng.h"

#include <iostream>

// Abstract class with interface to access color of material
struct Material
{
	float ka;
	float kd;
	float ks;
	float exponent;
	float reflectance;
	float transmittance;
	float refraction;

	Material(float ka, float kd, float ks, float exp, float refl, float trans, float refr) :
		ka(ka),
		kd(kd),
		ks(ks),
		exponent(exp),
		reflectance(refl),
		transmittance(trans),
		refraction(refr)
	{

	}

	virtual ~Material() { }
	virtual Color getColor(const Vector2f &pos) const = 0;
};

struct MaterialSolid : public Material
{
	Color color;

	MaterialSolid(const Color &color, float ka, float kd, float ks, float exp, float refl, float trans, float refr) :
		Material(ka, kd, ks, exp, refl, trans, refr),
		color(color)
	{

	}

	virtual Color getColor(const Vector2f &) const
	{
		return color;
	}
};

struct MaterialTextured : public Material
{
	unsigned width;
	unsigned height;
	std::vector <std::vector <Color>> texture;

	MaterialTextured(const std::string &filename, float ka, float kd, float ks, float exp, float refl, float trans, float refr) :
		Material(ka, kd, ks, exp, refl, trans, refr)
	{
		// Load texture
		std::vector <unsigned char> image;
		unsigned error = lodepng::decode(image, width, height, filename);
		if (error)
		{
			std::cerr << "PNG decoder error: " << lodepng_error_text(error) << std::endl;
			return;
		}

		texture.resize(width);
		for (size_t i = 0; i < width; ++i)
			texture[i].reserve(height);

		// Data returned from decode() is flat and we want it to be 2d
		for (size_t i = 0; i < image.size(); i += 4)
		{
			texture[(i / 4) % width].emplace_back(image[i] / 255.f, image[i + 1] / 255.f, image[i + 2] / 255.f);
		}
	}

	virtual Color getColor(const Vector2f &coord) const
	{
		size_t x = static_cast <size_t>(coord[0] * width);
		size_t y = static_cast <size_t>(coord[1] * height);
		x %= width;
		y %= height;

		return texture[x][y];
	}
};

#endif  // RAYTRACER_MATERIAL_H_