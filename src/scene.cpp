#include "scene.h"

#include "scene_parser.h"
#include "vector.h"

#include <vector>
#include <chrono>
#include <string>
#include <thread>
#include <future>
#include <random>
#include <atomic>

#ifdef LUA_BINDING_OFF
#define GET_POINTER(x) (x)
#else
#define GET_POINTER(x) (x).get()
#endif

using namespace std;

thread_local static bool inside = false;

struct RandomGenerator
{
	random_device rd;
	mt19937 randomEngine;
	uniform_real_distribution <float> dist;
	//static atomic <size_t> count;

	RandomGenerator() :
		rd(), randomEngine(rd()), dist(0.f, 1.f)
	{
		//count.fetch_add(1u);
		//cerr << count;
	}

	float next()
	{
		return dist(randomEngine);
	}
};

//atomic <size_t> RandomGenerator::count(0);

float getRandom()
{
	// We want to make unique copy for every thread because generator is not thread safe
	thread_local RandomGenerator rg;

	return rg.next();
}

Vector2f getRandomInRadius(float r)
{
	r = r * sqrt(getRandom());
	float theta = getRandom() * 2.f * static_cast <float>(M_PI);
	return { r * cos(theta), r * sin(theta) };
}

Scene::Scene() :
	pool(thread::hardware_concurrency()),
	properties(0)
{
}

Scene::~Scene()
{
#ifdef LUA_BINDING_OFF
	for (ObjectRef obj : objects)
		delete obj;

	for (Light *light : lights)
		delete light;
#endif
}

bool Scene::loadScene(const string &filename)
{
	SceneParser scene(filename);
	if (!scene)
		return false;

	for (Light *light : scene.getLights())
		lights.push_back(light);
	for (Object *obj : scene.getSurfaces())
		objects.push_back(obj);

	camera = scene.getCamera();
	background = scene.getBackgroundColor();
	outputFile = scene.getOutputFile();

	return true;
}

string Scene::getOutputFile() const
{
	return outputFile;
}

Color Scene::traceRay(const Ray &ray, size_t bounces) const
{
	Color res;

	const auto &[obj, inter] = findIntersection(ray);

	if (obj == nullptr)
		return background;

	Material *mat = GET_POINTER(obj->material);
	bool hitFromBehind = (inter.normal * ray.dir) > 0.f;
	for (size_t i = 0; i < lights.size(); ++i)
	{
		if (!lights[i]->isOn())
			continue;

		if (lights[i]->isDirectional())
		{
			// Cast ray from point of intersection to light source
			auto[lightDir, lightDist] = lights[i]->getDirection(inter.pos);
			Ray lightRay(inter.pos + inter.normal * (hitFromBehind ? -0.0001f : 0.0001f), lightDir);
			// Check if something is in the way of ray
			const auto &[obj, lightInter] = findIntersection(lightRay);
			if (obj != nullptr)
			{
				float dist = (lightInter.pos - inter.pos).length();
				if (lightDist > dist)
					continue;
			}
		}
		else if (inside)
		{
			// Do not apply ambient light inside objects
			continue;
		}		
		
		res += lights[i]->getColor(inter, ray.pos, mat);
	}

	res *= (1.f - mat->reflectance - mat->transmittance);

	if (bounces == 0)
		return res;

	if (mat->reflectance > 0.f)
	{
		Ray refl = ray.reflect(
			inter.pos,
			(inter.normal * ray.dir) > 0 ? -inter.normal : inter.normal,
			0.0001f);
		refl.dir.normalize();
		res += traceRay(refl, bounces - 1) * mat->reflectance;
	}

	if (mat->transmittance > 0.f)
	{
		auto [refr, refracted, negated] = ray.refract(inter.pos, inter.normal, 1.f, mat->refraction, 0.0001f);
		refr.dir.normalize();
		
		if (refracted)
			inside = !inside;
		res += traceRay(refr, bounces - 1) * mat->transmittance;
		if (refracted)
			inside = !inside;
	}

	return res;
}

Color Scene::traceReal(const Vector3f &d) const
{
	Color res;
	size_t rays = settings.at(Scene::DOF);
	Vector3f focalPoint = d * camera.getFocalLength();

	for (size_t i = 0; i < rays; ++i)
	{
		Vector3f r(getRandomInRadius(camera.getAperture()), 0.f);
		Vector3f dr = focalPoint - r;
		dr.normalize();
		dr = camera.getView() * dr;
		r = camera.getView() * r;

		Ray ray(camera.getPosition() + r, dr);
		inside = false;
		res += traceRay(ray, camera.getMaxBounces());
	}

	Vector3f dir = camera.getView() * d;
	Ray ray(camera.getPosition(), dir);
	inside = false;
	res += traceRay(ray, camera.getMaxBounces());

	res /= static_cast <float>(rays + 1);
	return res;
}

Color Scene::supersampleGrid(float xf, float yf, float dx, float dy, size_t sub) const
{
	Color res;
	float sub_xf = xf - (dx / 2.f) + (dx / (sub * 2));
	for (size_t sub_x = 0; sub_x < sub; ++sub_x)
	{
		float sub_yf = yf - (dy / 2.f) + (dy / (sub * 2));
		for (size_t sub_y = 0; sub_y < sub; ++sub_y)
		{
			Vector3f d(sub_xf, sub_yf, -1.f);
			d.normalize();
			if (hasProperty(Scene::DOF))
				res += traceReal(d);
			else
			{
				d = camera.getView() * d;
				Ray ray(camera.getPosition(), d);
				inside = false;
				res += traceRay(ray, camera.getMaxBounces());
			}

			sub_yf += dy / sub;
		}
		sub_xf += dx / sub;
	}
	res /= static_cast <float> (sub * sub);
	return res;
}

Color Scene::supersampleJitter(float xf, float yf, float dx, float dy, size_t sub) const
{
	Color res;

	float sub_xf = xf - dx / 2.f;
	for (size_t sub_x = 0; sub_x < sub; ++sub_x)
	{
		float sub_yf = yf - dy / 2.f;
		for (size_t sub_y = 0; sub_y < sub; ++sub_y)
		{
			Vector3f d(
				getRandom() * (dx / sub) + sub_xf,
				getRandom() * (dy / sub) + sub_yf,
				-1.f);

			d.normalize();

			if (hasProperty(Scene::DOF))
				res += traceReal(d);
			else
			{
				d = camera.getView() * d;
				Ray ray(camera.getPosition(), d);
				inside = false;
				res += traceRay(ray, camera.getMaxBounces());
			}

			sub_yf += dy / sub;
		}
		sub_xf += dx / sub;
	}
	res /= static_cast <float> (sub * sub);

	return res;
}

Color Scene::getPixel(float xf, float yf, float dx, float dy, size_t sub) const
{
	if (hasProperty(SupersamplingJitter))
	{
		return supersampleJitter(xf, yf, dx, dy, sub);
	}
	else if (hasProperty(SupersamplingGrid))
	{
		return supersampleGrid(xf, yf, dx, dy, sub);
	}
	else
	{
		Vector3f d(xf, yf, -1.f);
		d.normalize();
		if (hasProperty(DOF))
		{
			return traceReal(d);
		}

		d = camera.getView() * d;
		Ray ray(camera.getPosition(), d);
		inside = false;
		return traceRay(ray, camera.getMaxBounces());
	}
}

vector <vector <Color>> Scene::render()
{
	vector <vector <Color>> data;
	data.resize(camera.getResolution().first);

	// Check if transforms of objects were changed since last render call and update inverce matrices if needed
	for (ObjectRef obj: objects)
		obj->updateInverse();

	float ratio = static_cast <float>(camera.getResolution().first) / camera.getResolution().second;
	float xm = tan(camera.getFOV());
	float ym = tan(camera.getFOV() / ratio);

	// Distance between rays (on image plane)
	float dx = 2.f * xm / camera.getResolution().first;
	float dy = 2.f * ym / camera.getResolution().second;
	size_t subPixels = hasProperty(Supersampling) ? settings.at(Scene::Supersampling) : 0;

	// Generate rays
	for (size_t x = 0; x < camera.getResolution().first; ++x)
	{
		data[x].resize(camera.getResolution().second);
		
		float xf = static_cast <float>(x) / camera.getResolution().first;
		xf = (2.f * xf - 1.f) * xm;
		for (size_t y = 0; y < camera.getResolution().second; ++y)
		{
			float yf = static_cast <float>(y) / camera.getResolution().second;
			yf = (2.f * yf - 1.f) * ym;

			data[x][camera.getResolution().second - y - 1] = getPixel(xf, yf, dx, dy, subPixels);
		}
	}

	return data;
}

vector <vector <Color>> Scene::renderParallel()
{
	vector <vector <Color>> data;
	data.resize(camera.getResolution().first);

	// Check if transforms of objects were changed since last render call and update inverce matrices if needed
	for (ObjectRef &obj : objects)
		obj->updateInverse();

	float ratio = static_cast <float>(camera.getResolution().first) / camera.getResolution().second;
	float xm = tan(camera.getFOV());
	float ym = tan(camera.getFOV() / ratio);

	// Distance between rays
	float dx = 2.f * xm / camera.getResolution().first;
	float dy = 2.f * ym / camera.getResolution().second;
	size_t sub = hasProperty(Supersampling) ? settings.at(Scene::Supersampling) : 0;

	vector <future <vector <Color>>> fut;
	fut.reserve(camera.getResolution().first);

	for (size_t x = 0; x < camera.getResolution().first; ++x)
	{
		float xf = static_cast <float>(x) / camera.getResolution().first;
		xf = (2 * xf - 1) * xm;

		fut.push_back(pool.push([this](int id, float xf, float ym, float dx, float dy, size_t sub)
		{
			return this->traceColumn(xf, ym, dx, dy, sub);
		}, xf, ym, dx, dy, sub));
	}
	
	for (size_t i = 0; i < fut.size(); ++i)
		data[i] = fut[i].get();

	return data;
}

vector <Color> Scene::traceColumn(float xf, float ym, float dx, float dy, size_t sub) const
{
	vector <Color> data;

	data.resize(camera.getResolution().second);
	for (size_t y = 0; y < camera.getResolution().second; ++y)
	{
		float yf = static_cast <float>(y) / camera.getResolution().second;
		yf = (2 * yf - 1) * ym;
		data[camera.getResolution().second - y - 1] = getPixel(xf, yf, dx, dy, sub);
	}
	return data;
}

pair <Object *, Intersection> Scene::findIntersection(const Ray &ray) const
{
	float dist;
	Object *obj = nullptr;
	Intersection inter;
	// Iterate through all objects and find closest intersection
	for (size_t i = 0; i < objects.size(); ++i)
	{
		auto in = objects[i]->intersection(ray);
		if (in.first)
		{
			float d = (ray.pos - in.second.pos).sqrLength();
			if (obj == nullptr || d < dist)
			{
				dist = d;
				obj = GET_POINTER(objects[i]);
				inter = in.second;
			}
		}
	}

	return { obj, inter };
}

void Scene::setProperty(Property prop)
{
	properties |= prop;
}

bool Scene::hasProperty(Property prop) const
{
	return properties & prop;
}

Camera & Scene::getCameraRef()
{
	return camera;
}

Camera Scene::getCamera() const
{
	return camera;
}

void Scene::setCamera(const Camera &cam)
{
	camera = cam;
}

Scene::ObjectRef Scene::getObject(size_t n)
{
	return objects[n];
}

void Scene::addObject(ObjectRef obj)
{
	objects.push_back(obj);
}

bool Scene::deleteObject(ObjectRef obj)
{
	for (size_t i = 0; i < objects.size(); ++i)
	{
		if (GET_POINTER(objects[i]) == GET_POINTER(obj))
		{
			objects.erase(objects.begin() + i);
			return true;
		}
	}
	return false;
}

size_t Scene::objectsSize() const
{
	return objects.size();
}

Scene::LightRef Scene::getLight(size_t n)
{
	return lights[n];
}

void Scene::addLight(LightRef light)
{
	lights.push_back(light);
}

bool Scene::deleteLight(LightRef light)
{
	for (size_t i = 0; i < lights.size(); ++i)
	{
		if (GET_POINTER(lights[i]) == GET_POINTER(light))
		{
			lights.erase(lights.begin() + i);
			return true;
		}
	}
	return false;
}

size_t Scene::lightsSize() const
{
	return lights.size();
}
