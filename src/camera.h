#ifndef RAYTRACER_CAMERA_H_
#define RAYTRACER_CAMERA_H_

#include "matrix.h"
#include "vector.h"
#define _USE_MATH_DEFINES
#include <math.h>

class Camera
{
	Vector3f pos;
	Vector3f lookat;
	Vector3f up;
	float fov;
	std::pair <size_t, size_t> resolution;
	size_t bounces;
	Matrix4f view;

	float focalLength;
	float aperture;

	// Angle between two 2d vectors
	static float getAngle(const Vector2f &v1, const Vector2f &v2)
	{
		float dot = v1 * v2;
		float det = v1[0] * v2[1] - v1[1] * v2[0];
		return std::atan2(det, dot);
	}

public:

	Camera() {}

	Camera(const Vector3f &pos, const Vector3f &lookat, const Vector3f &up, float fov, std::pair <size_t, size_t> resolution, size_t bounces, float aperture = 0.f) :
		pos(pos),
		lookat(lookat),
		up(up),
		fov(fov * static_cast <float>(M_PI) / 180.f),
		resolution(resolution),
		bounces(bounces),
		aperture(aperture)
	{
		updateView();
	}

	// Generate (inverse) view matrix from position, lookat and up vector
	// This matrix can be used to transform rays from camera space to world space
	void updateView()
	{
		// Direction at which camera points
		Vector3f dir = lookat - pos;
		dir.normalize();

		// To direct object in specific direction we need three rotation matrices:
		// pitch (rotation around X), yaw (Y), and roll (Z)
		// Pitch and yaw will be generated with the help of vector dir

		// Yaw
		// Project direction on plane xz (y = 0)
		Vector2f dir_xz(dir[0], dir[2]);
		dir_xz.normalize();
		Vector2f xz(0.f, -1.f); // default camera looks in direction (0, 0, -1)
		// Get angle between default and given direction
		float axz = -getAngle(xz, dir_xz);
		auto rotY = Matrix4f::fromRotationY(axz);
		auto rotYi = Matrix4f::fromRotationY(-axz);

		// Pitch
		// Rotate direction around y and project on plane yz (x = 0)
		Vector3f dirX = rotYi * dir;
		Vector2f dir_yz(dirX[1], dirX[2]);
		dir_yz.normalize();
		Vector2f yz(0.f, -1.f);
		float ayz = getAngle(yz, dir_yz);
		auto rotX = Matrix4f::fromRotationX(ayz);
		auto rotXi = Matrix4f::fromRotationX(-ayz);
		
		// To calculate roll we have to transform up vector from world space to camera space
		// This makes it possible to project it on plane xy (z = 0)
		up.normalize();
		Vector3f upt = rotYi * rotXi * up;
		Vector2f dir_up(upt[0], upt[1]);
		dir_up.normalize();
		Vector2f xy(0.f, 1.f);
		float axy = getAngle(xy, dir_up);
		auto rotZ = Matrix4f::fromRotationZ(axy);

		view = rotX * rotY * rotZ;
		// I feel like there is simpler way to do this, but hey, as long as it works

		focalLength = (lookat - pos).length();
	}

	void lookAt(const Vector3f &pos, const Vector3f &lookat, const Vector3f &up)
	{
		this->pos = pos;
		this->lookat = lookat;
		this->up = up;
		updateView();
	}
	
	const Matrix4f getView() const
	{
		return view;
	}

	Vector3f getPosition() const
	{
		return pos;
	}

	void setPosition(const Vector3f &pos)
	{
		this->pos = pos;
		updateView();
	}

	Vector3f getUp() const
	{
		return up;
	}

	void setUp(const Vector3f &up)
	{
		this->up = up;
		updateView();
	}

	Vector3f getLookat() const
	{
		return lookat;
	}

	void setLookat(const Vector3f &lookat)
	{
		this->lookat = lookat;
		updateView();
	}

	float getFOV() const
	{
		return fov;
	}

	void setFOV(float fov)
	{
		this->fov = fov;
	}

	std::pair <size_t, size_t> getResolution() const
	{
		return resolution;
	}

	size_t getMaxBounces() const
	{
		return bounces;
	}

	float getFocalLength() const
	{
		return focalLength;
	}

	float getAperture() const
	{
		return aperture;
	}

	void setAperture(float apert)
	{
		aperture = apert;
	}
};

#endif  // RAYTRACER_CAMERA_H_