#ifndef RAYTRACER_SCENE_PARSER_H_
#define RAYTRACER_SCENE_PARSER_H_

#include "vector.h"
#include "color.h"
#include "matrix.h"
#include "light.h"
#include "material.h"
#include "camera.h"

#include "pugixml.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#undef TINYOBJLOADER_IMPLEMENTATION

#include <iostream>
#include <string>
#include <vector>

// Provides simple interface to access data in xml file
class SceneParser
{
	pugi::xml_document doc;
	pugi::xml_node root;
	pugi::xml_parse_result status;

	Color getColor(pugi::xml_node node)
	{
		return Color{
			node.attribute("r").as_float(),
			node.attribute("g").as_float(),
			node.attribute("b").as_float(),
		};
	}

	Vector3f getVector(pugi::xml_node node)
	{
		return Vector3f{
			node.attribute("x").as_float(),
			node.attribute("y").as_float(),
			node.attribute("z").as_float(),
		};
	}

	// Returns transform and its inverse
	std::pair <Matrix4f, Matrix4f> getTransform(pugi::xml_node node)
	{
		std::string name(node.name());
		if (name == "translate")
		{
			Vector3f tr = getVector(node);
			return { Matrix4f::fromTranslation(tr), Matrix4f::fromTranslation(-tr) };
		}
		else if (name == "scale")
		{
			Vector3f sc = getVector(node);
			Vector3f v(1.f, 1.f, 1.f);
			return { Matrix4f::fromScaling(sc), Matrix4f::fromScaling(v / sc) };
		}
		else if (name == "rotateX")
		{
			float theta = node.attribute("theta").as_float();
			theta = theta * static_cast <float>(M_PI) / 180.f;
			return { Matrix4f::fromRotationX(theta), Matrix4f::fromRotationX(-theta) };
		}
		else if (name == "rotateY")
		{
			float theta = node.attribute("theta").as_float();
			theta = theta * static_cast <float>(M_PI) / 180.f;
			return { Matrix4f::fromRotationY(theta), Matrix4f::fromRotationY(-theta) };
		}
		else if (name == "rotateZ")
		{
			float theta = node.attribute("theta").as_float();
			theta = theta * static_cast <float>(M_PI) / 180.f;
			return { Matrix4f::fromRotationZ(theta), Matrix4f::fromRotationZ(-theta) };
		}

		return { Matrix4f(), Matrix4f() };
	}
	
	std::pair <Matrix4f, Matrix4f> getTransforms(pugi::xml_node node)
	{
		Matrix4f transform;
		Matrix4f inverse;
		for (pugi::xml_node tr : node.children())
		{
			auto transforms = getTransform(tr);
			transform *= transforms.first;
			inverse = transforms.second * inverse;
		}

		return {transform, inverse};
	}

	Material *getMaterialSolid(pugi::xml_node node)
	{
		auto f = node.child("phong");
		return new MaterialSolid{
			getColor(node.child("color")),
			f.attribute("ka").as_float(),
			f.attribute("kd").as_float(),
			f.attribute("ks").as_float(),
			f.attribute("exponent").as_float(),
			node.child("reflectance").attribute("r").as_float(),
			node.child("transmittance").attribute("t").as_float(),
			node.child("refraction").attribute("iof").as_float()
		};
	}

	Material *getMaterialTextured(pugi::xml_node node)
	{
		auto f = node.child("phong");
		return new MaterialTextured{
			node.child("texture").attribute("name").as_string(),
			f.attribute("ka").as_float(),
			f.attribute("kd").as_float(),
			f.attribute("ks").as_float(),
			f.attribute("exponent").as_float(),
			node.child("reflectance").attribute("r").as_float(),
			node.child("transmittance").attribute("t").as_float(),
			node.child("refraction").attribute("iof").as_float()
		};
	}

	Material *getMaterial(pugi::xml_node node)
	{
		auto m = node.child("material_solid");
		if (m.type() != pugi::xml_node_type::node_null)
			return getMaterialSolid(m);
		else
		{
			m = node.child("material_textured");
			return getMaterialTextured(m);
		}
	}

public:
	SceneParser(const std::string &filename)
	{
		status = doc.load_file(filename.c_str());
		if (status)
			root = doc.child("scene");
	}

	operator bool() const
	{
		return bool(status);
	}

	Color getBackgroundColor()
	{
		return getColor(root.child("background_color"));
	}

	std::vector <Light *> getLights()
	{
		std::vector <Light *> res;

		auto lights = root.child("lights");
		for (auto it = lights.begin(); it != lights.end(); ++it)
		{
			std::string name(it->name());
			if (name == "ambient_light")
			{
				auto c = it->child("color");
				res.push_back(new AmbientLight(getColor(c)));
			}
			else if (name == "parallel_light")
			{
				auto c = it->child("color");
				auto d = it->child("direction");
				res.push_back(new ParallelLight(getColor(c), getVector(d)));
			}
			else if (name == "point_light")
			{
				auto c = it->child("color");
				auto p = it->child("position");
				res.push_back(new PointLight(getColor(c), getVector(p)));
			}
			else if (name == "spot_light")
			{
				auto f = it->child("falloff");
				res.push_back(new SpotLight(
					getColor(it->child("color")),
					getVector(it->child("position")),
					getVector(it->child("direction")),
					f.attribute("alpha1").as_float() * static_cast <float>(M_PI) / 180.f,
					f.attribute("alpha2").as_float() * static_cast <float>(M_PI) / 180.f));
			}
		}

		return res;
	}

	std::vector <Object *> getSurfaces()
	{
		std::vector <Object *> res;
		auto surfaces = root.child("surfaces");
		for (auto it = surfaces.begin(); it != surfaces.end(); ++it)
		{
			std::string name(it->name());
			if (name == "sphere")
			{
				float r = it->attribute("radius").as_float();
				Material *mat = getMaterial(*it);
				Vector3f pos = getVector(it->child("position"));
				auto [transform, inverse] = getTransforms(it->child("transform"));
				res.push_back(new Sphere(r, mat, transform * Matrix4f::fromTranslation(pos), Matrix4f::fromTranslation(-pos) * inverse));
			}
			else if (name == "mesh")
			{
				std::string filename = it->attribute("name").as_string();
				Material *mat = getMaterial(*it);
				auto[transform, inverse] = getTransforms(it->child("transform"));
				res.push_back(new Mesh(filename, mat, transform, inverse));
			}
		}
		return res;
	}

	Camera getCamera()
	{
		auto camera = root.child("camera");
		return Camera{
			getVector(camera.child("position")),
			getVector(camera.child("lookat")),
			getVector(camera.child("up")),
			camera.child("horizontal_fov").attribute("angle").as_float(),
			{
				camera.child("resolution").attribute("horizontal").as_uint(),
				camera.child("resolution").attribute("vertical").as_uint(),
			},
			camera.child("max_bounces").attribute("n").as_uint(),
			camera.child("aperture").attribute("r").as_float()
		};
	}

	std::string getOutputFile()
	{
		return root.attribute("output_file").as_string();
	}
};

#endif  // RAYTRACER_SCENE_PARSER_H_
