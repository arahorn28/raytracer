#ifndef RAYTRACER_RAY_H_
#define RAYTRACER_RAY_H_

#include "vector.h"

struct Ray
{
	Vector3f pos;
	Vector3f dir;

	Ray(const Vector3f &pos, const Vector3f &dir) :
		pos(pos), dir(dir)
	{

	}

	Ray reflect(const Vector3f &pos, const Vector3f &surfaceNormal, float offset = 0.f) const
	{
		Vector3f d = dir.reflect(surfaceNormal);
		return Ray(pos + surfaceNormal * offset, d);
	}

	std::tuple <Ray, bool, bool> refract(const Vector3f &pos, const Vector3f &normal, float iof1, float iof2, float offset = 0.f) const
	{
		auto [d, refracted, negated] = dir.refract(normal, iof1, iof2);
		if (negated)
		{
			return { Ray(pos + normal * (refracted ? offset : -offset), d), refracted, negated };
		}
		else
		{
			return { Ray(pos + normal * (refracted ? -offset : offset), d), refracted, negated };
		}

		//return { Ray(pos + d * offset, d), refracted, negated };
	}

};

#endif  // RAYTRACER_RAY_H_
