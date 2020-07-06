#ifndef RAYTRACER_OBJECT_H_
#define RAYTRACER_OBJECT_H_

#include "ray.h"
#include "material.h"
#include "matrix.h"
#include "vector.h"

#include "tiny_obj_loader.h"
#ifndef LUA_BINDING_OFF
#include "LuaBridge/RefCountedPtr.h"
#endif

#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>

// Struct that holds information about intersection
struct Intersection
{
	Vector3f pos;		// Point of intersection
	Vector3f normal;	// Surface normal in point of intersection
	Vector2f tex;		// Texture coordinates
};

// TODO
struct BoundingBox
{
	Vector3f min;
	Vector3f max;

	void resize(const Vector3f &a, const Vector3f &b, const Vector3f &c)
	{
		// TODO
	}
};


class Object
{
	bool changed;

protected:
	Matrix4f transform;
	Matrix4f inverseTransform;
	Matrix4f inverseTransposeTransform;

public:
#ifndef LUA_BINDING_OFF
	using MaterialRef = luabridge::RefCountedPtr <Material>;
#else
	using MaterialRef = Material * ;
#endif
	MaterialRef material;
	
	Object(Material *mat, const Matrix4f &transform = Matrix4f(), const Matrix4f &inverse = Matrix4f()) :
		changed(false),
		transform(transform),
		inverseTransform(inverse),
		inverseTransposeTransform(inverse.transpose()),
		material(mat)
	{
		
	}

	virtual ~Object()
	{
#ifdef LUA_BINDING_OFF
		delete material;
#endif 
	}

	virtual std::pair <bool, Intersection> intersection(const Ray &ray) = 0;

	Matrix4f getTransform() const
	{
		return transform;
	}

	void setTransform(const Matrix4f &m)
	{
		changed = true;
		transform = m;
	}
	
	// Update inverse of matrix if transform was changed
	void updateInverse()
	{
		if (changed)
		{
			inverseTransform = transform.invert();
			inverseTransposeTransform = inverseTransform.transpose();
			changed = false;
		}
	}

	MaterialRef getMaterial() const
	{
		return material;
	}

	void setMaterial(MaterialRef mat)
	{
		material = mat;
	}
};

class Sphere : public Object
{
	float r;

public:
	Sphere(float r, Material *mat, const Matrix4f &transform = Matrix4f(), const Matrix4f &inverse = Matrix4f()) :
		Object(mat, transform, inverse),
		r(r)
	{
		
	}

	virtual std::pair <bool, Intersection> intersection(const Ray &original)
	{
		const Vector3f pos = inverseTransform * original.pos;
		// Expand dir to Vector4f with 0 as last element to ignore translation
		const Vector3f dir = inverseTransform * Vector4f{original.dir, 0.f};

		float dot = pos * dir;
		float dirLen = dir.sqrLength();
		float posLen = pos.sqrLength();
		float root = dot * dot - dirLen * (posLen - r * r);

		if (root < 0.f)
		{
			return { false, Intersection() };
		}

		root = std::sqrt(root);

		float t1 = (-dot - root) / dirLen;
		float t2 = (-dot + root) / dirLen;

		if (t1 < 0.f && t2 < 0.f)
		{
			return { false, Intersection() };
		}

		float t = (t1 >= 0.f && t2 >= 0.f) ? std::min(t1, t2) : std::max(t1, t2);

		Vector3f inter = pos + dir * t;
		
		Vector3f normal = inverseTransposeTransform * Vector4f(inter, 0.f);
		normal.normalize();

		float th = std::atan(inter[1] / inter[0]);
		float ph = std::atan(inter[2] / r);
		Vector2f tex(th / (2.f * static_cast<float>(M_PI)), (static_cast<float>(M_PI) - ph) / static_cast<float>(M_PI));

		return { true, { transform * inter, normal, tex } };
	}

	float getR() const
	{
		return r;
	}

	void setR(float radius)
	{
		r = std::abs(radius);
	}
};

class Mesh : public Object
{
private:
	struct Index {
		int vertex_index;
		int normal_index;
		int texcoord_index;
	};

	std::vector <Vector3f> vertices;
	std::vector <Vector3f> normals;
	std::vector <Vector2f> texcoords;
	std::vector <Index> indices;

	template <size_t N>
	static std::vector <Vector <float, N>> toVector(const std::vector <float> &vec)
	{
		std::vector <Vector <float, N>> res;
		for (size_t i = 0; i < vec.size(); i += N)
		{
			Vector <float, N> v;
			std::copy(&vec[i], &vec[i] + N, v.data);
			res.push_back(v);
		}
		return res;
	}

public:

	Mesh(const std::string &filename, Material *mat, const Matrix4f &transform = Matrix4f(), const Matrix4f &inverse = Matrix4f()) :
		Object(mat, transform, inverse)
	{
		//Load obj
		std::string err;
		std::string warn;
		tinyobj::attrib_t attrib;
		std::vector <tinyobj::shape_t> shapes;
		std::vector <tinyobj::material_t> materials;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());

		if (!err.empty())
			std::cerr << err << std::endl;

		if (!ret)
			return;

		// Reshape flat data into convenient format
		vertices = toVector<3>(attrib.vertices);
		normals = toVector<3>(attrib.normals);
		texcoords = toVector<2>(attrib.texcoords);
		for (size_t i = 0; i < shapes.size(); ++i)
		{
			for (size_t j = 0; j < shapes[i].mesh.indices.size(); ++j)
			{
				indices.push_back(Mesh::Index{
					shapes[i].mesh.indices[j].vertex_index,
					shapes[i].mesh.indices[j].normal_index,
					shapes[i].mesh.indices[j].texcoord_index });
			}
		}
	}

	virtual std::pair <bool, Intersection> intersection(const Ray &original)
	{
		const Vector3f pos = inverseTransform * original.pos;
		// Expand dir to Vector4f with 0 as last element to ignore translation
		const Vector3f dir = inverseTransform * Vector4f{ original.dir, 0.f };

		float t_min = -1.f;
		float uf = 0.f;
		float vf = 0.f;
		size_t ind = 0;

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			const Vector3f &A = vertices[indices[i].vertex_index];

			Vector3f E1 = vertices[indices[i + 1].vertex_index] - A;
			Vector3f E2 = vertices[indices[i + 2].vertex_index] - A;
			
			Vector3f P = dir.cross(E2);
			
			float k = P * E1;
			
			if (k > -0.00001f && k < 0.00001f)
				continue;

			//if (k < 0.f)
			//	continue;
			
			Vector3f T = pos - A;

			float u = (P * T) / k;
			if (u < -0.0001f || u > 1.0001f)
				continue;
			
			Vector3f Q = T.cross(E1);
			float v = (Q * dir) / k;
			if (v < -0.0001f || v > 1.0001f || (u + v) > 1.0001f)
				continue;
			
			float t = (Q * E2) / k;
			if (t < 0.f)
				continue;

			if (t < t_min || t_min < 0.f)
			{
				t_min = t;
				uf = u;
				vf = v;
				ind = i;
			}
		}

		if (t_min < 0.f)
			return { false, Intersection() };

		Vector3f inter = pos + dir * t_min;
		Vector3f normal = normals[indices[ind].normal_index] * (1.f - uf - vf) +
			normals[indices[ind + 1].normal_index] * uf +
			normals[indices[ind + 2].normal_index] * vf;
		normal = inverseTransposeTransform * Vector4f(normal, 0.f);
		normal.normalize();

		Vector2f tex = (texcoords.size() != 0) ? (texcoords[indices[ind].texcoord_index] * (1.f - uf - vf) +
			texcoords[indices[ind + 1].texcoord_index] * uf +
			texcoords[indices[ind + 2].texcoord_index] * vf) : Vector2f();

		return { true, { transform * inter, normal, tex } };
	}
};

#endif  // RAYTRACER_OBJECT_H_
