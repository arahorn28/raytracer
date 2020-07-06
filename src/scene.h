#ifndef RAYTRACER_SCENE_H_
#define RAYTRACER_SCENE_H_

#include "object.h"
#include "light.h"
#include "camera.h"
#include "color.h"

#include "ctpl_stl.h"

//#define LUA_BINDING_OFF

#ifndef LUA_BINDING_OFF
#include "LuaBridge/RefCountedPtr.h"
#endif

// Contains all information about scene
class Scene
{
public:
	enum Property {
		SupersamplingGrid = 1,
		SupersamplingJitter = 2,
		Supersampling = 3,
		DOF = 4
	};

#ifndef LUA_BINDING_OFF
	using ObjectRef = luabridge::RefCountedPtr <Object>;
	using LightRef = luabridge::RefCountedPtr <Light>;
#else
	using ObjectRef = Object * ;
	using LightRef = Light * ;
#endif

private:
	unsigned properties;
	Color background;
	Camera camera;
	std::vector <LightRef> lights;
	std::vector <ObjectRef> objects;
	std::string outputFile;
	ctpl::thread_pool pool;

	Color traceRay(const Ray &ray, size_t bounces = 0) const;
	std::pair <Object *, Intersection> findIntersection(const Ray &ray) const;
	Color traceReal(const Vector3f &dir) const;
	Color supersampleGrid(float xf, float yf, float dx, float dy, size_t sub) const;
	Color supersampleJitter(float xf, float yf, float dx, float dy, size_t sub) const;
	Color getPixel(float xf, float yf, float dx, float dy, size_t sub) const;

	std::vector <Color> traceColumn(float, float, float, float, size_t) const;

public:
	std::unordered_map <Property, size_t> settings;

	Scene();
	~Scene();

	bool loadScene(const std::string &filename);
	std::vector <std::vector <Color>> render();
	std::vector <std::vector <Color>> renderParallel();
	std::string getOutputFile() const;

	void setProperty(Property prop);
	bool hasProperty(Property prop) const;

	Camera & getCameraRef();
	Camera getCamera() const;
	void setCamera(const Camera &);

	ObjectRef getObject(size_t n);
	void addObject(ObjectRef obj);
	bool deleteObject(ObjectRef obj);
	size_t objectsSize() const;

	LightRef getLight(size_t n);
	void addLight(LightRef light);
	bool deleteLight(LightRef light);
	size_t lightsSize() const;
};

#endif  // RAYTRACER_SCENE_H_
