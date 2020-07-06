#ifndef RAYTRACER_COLOR_H_
#define RAYTRACER_COLOR_H_

#include "vector.h"

struct Color : public Vector3f
{
	Color() {}

	template <typename ...Args>
	Color(Args ... args) : Vector{ args... }
	{

	}

	using Vector3f::operator*;
	using Vector3f::operator*=;

	Color operator *(const Color &rhs) const
	{
		Color res;
		for (size_t i = 0; i < 3; ++i)
		{
			res[i] = data[i] * rhs.data[i];
		}
		return res;
	}

	void operator *=(const Color &rhs)
	{
		for (size_t i = 0; i < 3; ++i)
		{
			data[i] *= rhs.data[i];
		}
	}
};

#endif  // RAYTRACER_COLOR_H_