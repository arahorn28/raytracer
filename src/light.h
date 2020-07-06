#ifndef RAYTRACER_LIGHT_H_
#define RAYTRACER_LIGHT_H_

#include "color.h"
#include "material.h"
#include "object.h"
#include "vector.h"

// TODO add possibility to change lighting model
// Template class Light<LightingModel> ?

struct Light
{
private:
	// Indicates if light has direction
	// Used only to distinguish ambient light from other
	bool directional;
	bool on;

public:
	Color color;

public:
	
	Light() {}

	Light(const Color &c, bool directional = true, bool isOn = true) :
		directional(directional),
		on(isOn),
		color(c)
	{

	}

	virtual ~Light() {}

	// Apply light to some point of intersection with given material
	virtual Color getColor(const Intersection &inter, const Vector3f &camera, const Material *mat) const = 0;
	
	// Get direction and distance to light source
	virtual std::pair <Vector3f, float> getDirection(const Vector3f &inter) const = 0;

	bool isDirectional()
	{
		return directional;
	}

	bool isOn() const
	{
		return on;
	}

	void setOn(bool isOn)
	{
		on = isOn;
	}
};

struct AmbientLight : public Light
{
	AmbientLight() {}

	AmbientLight(const Color &c) : Light(c, false)
	{

	}

	virtual Color getColor(const Intersection &inter, const Vector3f &camera, const Material *mat) const
	{
		return mat->getColor(inter.tex) * color * mat->ka;
	}

	virtual std::pair <Vector3f, float> getDirection(const Vector3f &inter) const
	{
		return { Vector3f(), 0.f };
	}
};

struct ParallelLight : public Light
{
	Vector3f direction;

	ParallelLight() {}

	ParallelLight(const Color &c, const Vector3f &d) : Light(c), direction(d)
	{
		direction.normalize();
	}

	virtual Color getColor(const Intersection &inter, const Vector3f &camera, const Material *mat) const
	{
		Vector3f toLight(-direction);
		Vector3f toCamera = camera - inter.pos;
		toCamera.normalize();

		Color matColor = mat->getColor(inter.tex);
		// Diffuse
		Color dif = matColor * color * (std::max(0.f, inter.normal * toLight) * mat->kd);
		// Specular
		Vector3f R = direction.reflect(inter.normal);
		R.normalize();
		Color spec =  color * (std::pow(std::max(0.f, R * toCamera), mat->exponent) * mat->ks);

		return dif + spec;
	}

	virtual std::pair <Vector3f, float> getDirection(const Vector3f &inter) const
	{
		return { -direction, std::numeric_limits <float>::infinity() };
	}
};

struct PointLight : public Light
{
	Vector3f position;

	PointLight() {}

	PointLight(const Color &c, const Vector3f &p) :
		Light(c),
		position(p)
	{

	}

	virtual Color getColor(const Intersection &inter, const Vector3f &camera, const Material *mat) const
	{
		Vector3f toLight = position - inter.pos;
		toLight.normalize();
		Vector3f toCamera = camera - inter.pos;
		toCamera.normalize();

		Color matColor = mat->getColor(inter.tex);
		// Diffuse
		Color dif = matColor * color * (std::max(0.f, inter.normal * toLight) * mat->kd);
		// Specular
		Vector3f R = (-toLight).reflect(inter.normal);
		R.normalize();
		Color spec = color * (std::pow(std::max(0.f, R * toCamera), mat->exponent) * mat->ks);

		return dif + spec;
	}

	virtual std::pair <Vector3f, float> getDirection(const Vector3f &inter) const
	{
		Vector3f d = position - inter;
		float dist = d.length();
		d.normalize();

		return { d, dist };
	}
};

struct SpotLight : public Light
{
	Vector3f position;
	Vector3f direction;
	float inner, outer;

	SpotLight() {}

	SpotLight(const Color &color, const Vector3f &pos, const Vector3f &dir, float inner, float outer) :
		Light(color),
		position(pos),
		direction(dir),
		inner(std::cos(inner)),
		outer(std::cos(outer))
	{
		direction.normalize();
	}

	virtual Color getColor(const Intersection &inter, const Vector3f &camera, const Material *mat) const
	{
		Vector3f toLight = position - inter.pos;
		toLight.normalize();
		Vector3f toCamera = camera - inter.pos;
		toCamera.normalize();

		Color matColor = mat->getColor(inter.tex);
		// Diffuse
		Color dif = matColor * color * (std::max(0.f, inter.normal * toLight) * mat->kd);
		// Specular
		Vector3f R = (-toLight).reflect(inter.normal);
		R.normalize();
		Color spec = color * (std::pow(std::max(0.f, R * toCamera), mat->exponent) * mat->ks);
		// Hermite interpolation
		float a = direction * (-toLight);
		float k = (a - outer) / (inner - outer);
		k = std::max(0.f, k);
		k = std::min(1.f, k);
		k *= k * (3.f - 2.f * k);

		return (dif + spec) * k;
	}

	virtual std::pair <Vector3f, float> getDirection(const Vector3f &inter) const
	{
		Vector3f d = position - inter;
		float dist = d.length();
		d.normalize();

		return { d, dist };
	}

	Vector3f getDir() const
	{
		return direction;
	}

	void setDir(const Vector3f &dir)
	{
		direction = dir;
		direction.normalize();
	}
};

#endif  // RAYTRACER_LIGHT_H_