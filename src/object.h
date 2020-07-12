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

#include <limits>
#include <algorithm>
#include <stack>
#define _USE_MATH_DEFINES
#include <math.h>

//#define BOUNDING_BOX
#define OBJECT_BOUNDING_TREE

#define VERTEX_IN_BOUNDS(v, minv, maxv) \
	((v)[0] >= (minv)[0] &&  (v)[1] >= (minv)[1] && (v)[2] >= (minv)[2] &&  \
	(v)[0] <= (maxv)[0] && (v)[1] <= (maxv)[1] && (v)[2] <= (maxv)[2])


// Struct that holds information about intersection
struct Intersection
{
	Vector3f pos;		// Point of intersection
	Vector3f normal;	// Surface normal in point of intersection
	Vector2f tex;		// Texture coordinates
};

struct VertexIndices {
	int vertex_index;
	int normal_index;
	int texcoord_index;
};


class BoundingBox
{
	Vector3f min;
	Vector3f max;

	std::vector <size_t> indices;

	BoundingBox* left;
	BoundingBox* right;

	// Merge two sorted vectors with removal of duplicates
	static std::vector <size_t> merge(const std::vector<size_t>& v1, const std::vector<size_t>& v2)
	{
		std::vector <size_t> res;
		size_t i1 = 0, i2 = 0;
		size_t s1 = v1.size(), s2 = v2.size();
		if (s1 == 0)
		{
			res.reserve(s2);
			std::copy(v2.cbegin(), v2.cend(), std::back_inserter(res));
			return res;
		}
		if (s2 == 0)
		{
			res.reserve(s1);
			std::copy(v1.cbegin(), v1.cend(), std::back_inserter(res));
			return res;
		}
		while (true)
		{
			if (v1[i1] < v2[i2])
			{
				res.push_back(v1[i1]);
				++i1;
				if (i1 == s1)
				{
					std::copy(v2.cbegin() + i2, v2.cend(), std::back_inserter(res));
					return res;
				}
			}
			else if (v1[i1] > v2[i2])
			{
				res.push_back(v2[i2]);
				++i2;
				if (i2 == s2)
				{
					std::copy(v1.cbegin() + i1, v1.cend(), std::back_inserter(res));
					return res;
				}
			}
			else
			{
				res.push_back(v1[i1]);
				++i1;
				++i2;
				if (i1 == s1)
				{
					std::copy(v2.cbegin() + i2, v2.cend(), std::back_inserter(res));
					return res;
				}
				if (i2 == s2)
				{
					std::copy(v1.cbegin() + i1, v1.cend(), std::back_inserter(res));
					return res;
				}
			}
		}
	}

public:
	enum class Axis : size_t { X = 0, Y = 1, Z = 2 };

	BoundingBox() :
		min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
		max(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()),
		left(nullptr),
		right(nullptr)
	{

	}

	BoundingBox(Vector3f min, Vector3f max) :
		min(min),
		max(max),
		left(nullptr),
		right(nullptr)
	{

	}

	~BoundingBox()
	{
		if (left != nullptr)
			delete left;
		if (right != nullptr)
			delete right;
	}

	void addIndices(Axis axis, const std::vector <size_t>& indices,
		const std::vector <VertexIndices>& vertexIndices, const std::vector <Vector3f>& vertices)
	{
		if (indices.size() <= 50)
		{
			this->indices = indices;
			return;
		}
		Vector3f midBottom{ min }, midTop{ max };
		midBottom[static_cast<size_t>(axis)] =
			(midBottom[static_cast<size_t>(axis)] + max[static_cast<size_t>(axis)]) / 2.f;
		midTop[static_cast<size_t>(axis)] =
			(midTop[static_cast<size_t>(axis)] + min[static_cast<size_t>(axis)]) / 2.f;

		std::vector <size_t> leftBoxIndices, rightBoxIndices;
		// Make actual size of children boxes a little bit bigger along the current axis
		float axisExtension = std::fabs((min[static_cast<size_t>(axis)] + max[static_cast<size_t>(axis)]) * 0.005f / 2.f);
		Vector3f minExtended{ min };
		Vector3f maxExtended{ max };
		Vector3f midBottomExtended{ midBottom };
		Vector3f midTopExtended{ midTop };
		minExtended[static_cast<size_t>(axis)] -= axisExtension;
		maxExtended[static_cast<size_t>(axis)] += axisExtension;
		midBottomExtended[static_cast<size_t>(axis)] -= axisExtension;
		midTopExtended[static_cast<size_t>(axis)] += axisExtension;
		for (size_t i = 0; i < indices.size(); ++i)
		{
			const Vector3f v1 = vertices[vertexIndices[indices[i]].vertex_index],
				v2 = vertices[vertexIndices[indices[i] + 1].vertex_index],
				v3 = vertices[vertexIndices[indices[i] + 2].vertex_index];

			if (VERTEX_IN_BOUNDS(v1, minExtended, midTopExtended) ||
				VERTEX_IN_BOUNDS(v2, minExtended, midTopExtended) ||
				VERTEX_IN_BOUNDS(v3, minExtended, midTopExtended))
			{
				leftBoxIndices.push_back(indices[i]);
			}
			if (VERTEX_IN_BOUNDS(v1, midBottomExtended, maxExtended) ||
				VERTEX_IN_BOUNDS(v2, midBottomExtended, maxExtended) ||
				VERTEX_IN_BOUNDS(v3, midBottomExtended, maxExtended))
			{
				rightBoxIndices.push_back(indices[i]);
			}
		}
		left = new BoundingBox(minExtended, midTopExtended);
		right = new BoundingBox(midBottomExtended, maxExtended);
		// Recursive tree construction
		left->addIndices(Axis{ (static_cast<size_t>(axis) + 1) % 3 }, leftBoxIndices, vertexIndices, vertices);
		right->addIndices(Axis{ (static_cast<size_t>(axis) + 1) % 3 }, rightBoxIndices, vertexIndices, vertices);
	}

	// Expand box to cantain given point
	void expand(const Vector3f& point)
	{
		for (size_t i = 0; i < 3; ++i)
		{
			if (point[i] < min[i])
				min[i] = point[i];
			if (point[i] > max[i])
				max[i] = point[i];
		}
	}

	// Test if ray intersects box
	bool intersects(const Ray& ray) const
	{
		Vector3f min, max;
		Vector3f inv{ 1 / ray.dir[0], 1 / ray.dir[1], 1 / ray.dir[2] };

		if (inv[0] < 0.f)
		{
			min[0] = (this->max[0] - ray.pos[0]) * inv[0];
			max[0] = (this->min[0] - ray.pos[0]) * inv[0];
		}
		else
		{
			min[0] = (this->min[0] - ray.pos[0]) * inv[0];
			max[0] = (this->max[0] - ray.pos[0]) * inv[0];
		}

		if (inv[1] < 0.f)
		{
			min[1] = (this->max[1] - ray.pos[1]) * inv[1];
			max[1] = (this->min[1] - ray.pos[1]) * inv[1];
		}
		else
		{
			min[1] = (this->min[1] - ray.pos[1]) * inv[1];
			max[1] = (this->max[1] - ray.pos[1]) * inv[1];
		}

		if ((min[0] > max[1]) || (min[1] > max[0]))
			return false;
		if (min[1] > min[0])
			min[0] = min[1];
		if (max[1] < max[0])
			max[0] = max[1];

		if (inv[2] < 0.f)
		{
			min[2] = (this->max[2] - ray.pos[2]) * inv[2];
			max[2] = (this->min[2] - ray.pos[2]) * inv[2];
		}
		else
		{
			min[2] = (this->min[2] - ray.pos[2]) * inv[2];
			max[2] = (this->max[2] - ray.pos[2]) * inv[2];
		}

		if ((min[0] > max[2]) || (min[2] > max[0]))
			return false;
		if (min[2] > min[0])
			min[0] = min[2];
		if (max[2] < max[0])
			max[0] = max[2];

		if (min[0] < 0)
		{
			if (max[0] < 0)
				return false;
		}

		return true;
	}

	// Get all polygons that are contained in boxes intersected by a ray.
	// Returns pair where first member is pointer to vector of indices and second 
	// is boolean that says if vector can be deleted by the caller
	std::pair <std::vector <size_t>*, bool> traverse(const Ray& ray)
	{
		if (left == nullptr)
			return { &indices, false };

		std::stack <BoundingBox*> boxes;
		boxes.push(this);
		std::vector <size_t> res;
		
		while (!boxes.empty())
		{
			BoundingBox* box = boxes.top();
			boxes.pop();
			if (box->intersects(ray))
			{
				if (box->left == nullptr && box->right == nullptr)
				{
					res = merge(res, box->indices);
				}
				else
				{
					boxes.push(box->left);
					boxes.push(box->right);
				}
			}
		}
		std::vector <size_t> *temp = new std::vector <size_t>();
		temp->swap(res);
		return { temp, true };
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
	std::vector <Vector3f> vertices;
	std::vector <Vector3f> normals;
	std::vector <Vector2f> texcoords;
	std::vector <VertexIndices> indices;
#if defined(BOUNDING_BOX) || defined(OBJECT_BOUNDING_TREE)
	BoundingBox box;
#endif

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
#if defined(BOUNDING_BOX) || defined(OBJECT_BOUNDING_TREE)
		//, box(new BoundingBox())
#endif
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
				indices.push_back(VertexIndices{
					shapes[i].mesh.indices[j].vertex_index,
					shapes[i].mesh.indices[j].normal_index,
					shapes[i].mesh.indices[j].texcoord_index });
			}
		}
#if defined(BOUNDING_BOX) || defined(OBJECT_BOUNDING_TREE)
		for (size_t i = 0; i < indices.size(); ++i)
		{
			box.expand(vertices[indices[i].vertex_index]);
		}
#ifdef OBJECT_BOUNDING_TREE
		std::vector <size_t> vertexIndices;
		vertexIndices.reserve(indices.size() / 3);
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			vertexIndices.push_back(i);
		}
		box.addIndices(BoundingBox::Axis::X, vertexIndices, indices, vertices);
#endif // OBJECT_BOUNDING_TREE
#endif // BOUNDING_BOX
	}
	
	virtual ~Mesh()
	{
#if defined(BOUNDING_BOX) || defined(OBJECT_BOUNDING_TREE)
		//delete box;
#endif
	}

	virtual std::pair <bool, Intersection> intersection(const Ray &original)
	{
		const Vector3f pos = inverseTransform * original.pos;
		// Expand dir to Vector4f with 0 as last element to ignore translation
		const Vector3f dir = inverseTransform * Vector4f{ original.dir, 0.f };

#ifdef BOUNDING_BOX
#ifndef OBJECT_BOUNDING_TREE
		if (!box.intersection({pos, dir}))
		{
			return { false, Intersection() };
		}
#endif
#endif // BOUNDING_BOX
#ifdef OBJECT_BOUNDING_TREE
		auto [possibleVertices, del] = box.traverse({ pos, dir });
		if (possibleVertices->empty())
		{
			return { false, Intersection() };
		}
#endif // OBJECT_BOUNDING_TREE

		float t_min = -1.f;
		float uf = 0.f;
		float vf = 0.f;
		size_t ind = 0;

#ifdef OBJECT_BOUNDING_TREE
		for (size_t j = 0, i; j < possibleVertices->size(); ++j)
		{
			i = possibleVertices->at(j);
#else
		for (size_t i = 0; i < indices.size(); i += 3)
		{
#endif
		
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
#ifdef OBJECT_BOUNDING_TREE
		if (del)
			delete possibleVertices;
#endif
		
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
