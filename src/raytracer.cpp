#include "scene.h"
#include "color.h"

#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "LuaBridge/RefCountedPtr.h"
#include "ctpl_stl.h"
#include "lodepng.h"
#include "cxxopts.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>
#include <chrono>
#include <future>
#include <map>
#ifndef _MSC_VER
#include <unistd.h>
#include <sys/stat.h>
#include <poll.h>
#endif

#ifndef ASYNC_WRITE_OFF
#define ASYNC_WRITE
#endif
#ifndef ASYNC_RENDER_OFF
#define ASYNC_RENDER
#endif

#define CAT(A, B) A##B
#ifdef _MSC_VER
#define chdir(T) _wchdir(CAT(L, T))
#define mkdir(T) _wmkdir(CAT(L, T))
#define POPEN(T) _popen((T), "wb")
#define PCLOSE _pclose
#else
//#define chdir(T) chdir(T)
#define POPEN(T) popen((T), "w")
#define PCLOSE pclose
#define mkdir(T) mkdir((T), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif

#define ADD_LUA_VAR(L, space, var) auto * CAT(__pointer_, var) = &(var); \
	getGlobalNamespace(L).beginNamespace(space).addVariable(#var, &CAT(__pointer_, var)).endNamespace()

using namespace std;
using namespace luabridge;

// Clamp value in range [0, 1]
float clamp(float val)
{
	val = min(val, 1.f);
	val = max(val, 0.f);

	return val;
}

unsigned char convertToChar(float val)
{
	val = clamp(val) * 255.f;
	return static_cast <unsigned char>(val);
}

vector <unsigned char> flattenImageData(const vector <vector <Color>>& data, size_t width, size_t height)
{
	vector <unsigned char> flatData(width * height * 3);

	for (size_t y = 0; y < height; ++y)
	{
		for (size_t x = 0; x < width; ++x)
		{
			flatData[(y * width + x) * 3 + 0] = convertToChar(data[x][y][0]);
			flatData[(y * width + x) * 3 + 1] = convertToChar(data[x][y][1]);
			flatData[(y * width + x) * 3 + 2] = convertToChar(data[x][y][2]);
		}
	}
	return flatData;
}

//Write image to disk
unsigned int writeImage(const vector <vector <Color>> &data, const string &filename)
{
	size_t width = data.size();
	if (data.size() == 0)
		return 1;
	size_t height = data[0].size();

	vector <unsigned char> flatData = flattenImageData(data, width, height);

	return lodepng::encode(filename, flatData, width, height, LodePNGColorType::LCT_RGB);
}

// Wirte image to FILE
unsigned int writeToFile(const vector <vector <Color>> &data, FILE *file)
{
	size_t width = data.size();
	if (width == 0)
		return 1;
	size_t height = data[0].size();

	vector <unsigned char> flatData = flattenImageData(data, width, height);

#ifdef __linux__
	// Check if ffmpeg is indeed running
	pollfd* pfds = new pollfd[1];
	pfds[0].fd = fileno(file);
	pfds[0].events = POLLOUT;
	int ready = poll(pfds, 1, -1);
	if (ready == 1 && (pfds[0].revents & POLLERR))
	{
		cerr << "\nCouldn't send data to pipe\nCheck if path to ffmpeg is correct\n";
		exit(0);
	}
#endif

	vector <unsigned char> res;
	lodepng::encode(res, flatData, width, height, LodePNGColorType::LCT_RGB);
	return fwrite(&res[0], 1, res.size(), file);
}

string getRelativePath(const string &binary)
{
	size_t pos = binary.find_last_of("\\/");
	if (pos == string::npos)
		return "";
	else
		return binary.substr(0, pos + 1);
}

string getOutputVideoName(const string &path)
{
	size_t pos = path.rfind('.');
	string res = path.substr(0, pos);
	return (res + ".mp4");
}

bool runScript(lua_State *L)
{
	if (lua_pcall(L, 0, 0, 0))
	{
		cerr << "Could execute script: " << lua_tostring(L, -1) << endl;
		return false;
	}

	LuaRef func = getGlobal(L, "tick");
	if (func.isNil() || !func.isFunction())
	{
		cerr << "Script does not contain function \"tick\"\n";
		return false;
	}

	return true;
}

lua_State *initScript(const string &filename)
{
	lua_State *L = luaL_newstate();
	if (!L)
	{
		cerr << "Could not init Lua\n";
		return nullptr;
	}

	if (luaL_loadfile(L, filename.c_str()))
	{
		cerr << filename << endl;
		cerr << "Could not load file: " << lua_tostring(L, -1) << endl;
		return nullptr;
	}
	luaL_openlibs(L);

	return L;
}

template <typename ...Args>
bool callLuaFunction(LuaRef f, Args... args)
{
	try
	{
		bool stop = f(args...).template cast<bool>();
		return stop;
	}
	catch (const exception &e)
	{
		cerr << "Exception during script execution: \n";
		cerr << e.what() << endl;
		return true;
	}
	catch (...)
	{
		cerr << "Exception during script execution\n";
		return true;
	}
}

void loadScriptingAPI(lua_State *L)
{
	getGlobalNamespace(L)
		.beginNamespace("tracer")
			.beginClass <Vector3f>("Vec")
				.addConstructor <void(*) (float, float, float)>()
				.addFunction("__add", (Vector3f(Vector3f::*)(const Vector3f &) const) &Vector3f::operator+)
				.addFunction("__sub", (Vector3f(Vector3f::*)(const Vector3f &) const) &Vector3f::operator-)
				.addFunction("__mul", (Vector3f(Vector3f::*)(const float &) const) &Vector3f::operator*)
				.addFunction("dot", &Vector3f::dot)
				.addFunction("__index", (const float & (Vector3f::*)(size_t) const) &Vector3f::operator[])
				//.addFunction("__newindex", (float & (Vector3f::*)(size_t)) &Vector3f::operator[])
				.addFunction("normalize", &Vector3f::normalize)
			.endClass()
			.deriveClass <Color, Vector3f>("Color")
				.addConstructor <void(*) (float, float, float)>()
				.addFunction("__mul", (Color(Color::*)(const Color &) const) &Color::operator*)
			.endClass()
			.beginClass <Matrix4f>("Mat")
				.addConstructor <void(*) ()>()
				.addFunction("__mul", (Matrix4f(Matrix4f::*)(const Matrix4f &) const) &Matrix4f::operator*)
				.addFunction("mul", (Vector3f(Matrix4f::*)(const Vector3f &) const) &Matrix4f::operator*)
				.addStaticFunction("fromTranslation", &Matrix4f::fromTranslation)
				.addStaticFunction("fromScaling", &Matrix4f::fromScaling)
				.addStaticFunction("fromRotationX", &Matrix4f::fromRotationX<Matrix4f>)
				.addStaticFunction("fromRotationY", &Matrix4f::fromRotationY<Matrix4f>)
				.addStaticFunction("fromRotationZ", &Matrix4f::fromRotationZ<Matrix4f>)
			.endClass()
			.beginClass <Camera>("Camera")
				.addFunction("lookAt", &Camera::lookAt)
				.addProperty("lookat", &Camera::getLookat, &Camera::setLookat)
				.addProperty("position", &Camera::getPosition, &Camera::setPosition)
				.addProperty("up", &Camera::getUp, &Camera::setUp)
				.addProperty("FOV", &Camera::getFOV, &Camera::setFOV)
				.addProperty("aperture", &Camera::getAperture, &Camera::setAperture)
			.endClass()
			.beginClass <Material>("Material")
				.addData("ka", &Material::ka)
				.addData("kd", &Material::kd)
				.addData("ks", &Material::ks)
				.addData("exp", &Material::exponent)
				.addData("reflectance", &Material::reflectance)
				.addData("transmittance", &Material::transmittance)
				.addData("iof", &Material::refraction)
			.endClass()
			.deriveClass <MaterialSolid, Material>("MaterialSolid")
				.addConstructor <void(*) (const Color &, float, float, float, float, float, float, float), RefCountedPtr <MaterialSolid>>()
				.addData("color", &MaterialSolid::color)
			.endClass()
			.deriveClass <MaterialTextured, Material>("MaterialTextured")
				.addConstructor <void(*) (const std::string &, float, float, float, float, float, float, float), RefCountedPtr <MaterialTextured>>()
			.endClass()
			.beginClass <Object>("Object")
				.addProperty("transform", &Object::getTransform, &Object::setTransform)
				.addProperty("material", &Object::getMaterial, &Object::setMaterial)
			.endClass()
			.deriveClass <Sphere, Object>("Sphere")
				.addConstructor <void(*) (float, Material *, const Matrix4f &, const Matrix4f &), RefCountedPtr <Sphere>>()
				.addProperty("r", &Sphere::getR, &Sphere::setR)
			.endClass()
			.deriveClass <Mesh, Object>("Mesh")
				.addConstructor <void(*) (const string &, Material *, const Matrix4f &, const Matrix4f &), RefCountedPtr <Mesh>>()
			.endClass()
			.beginClass <Light>("Light")
				.addProperty("on", &Light::isOn, &Light::setOn)
				.addData("color", &Light::color)
			.endClass()
			.deriveClass <AmbientLight, Light>("AmbientLight")
			.endClass()
			.deriveClass <ParallelLight, Light>("ParallelLight")
				.addConstructor <void(*) (const Color &, const Vector3f &), RefCountedPtr <ParallelLight>>()
				.addData("direction", &ParallelLight::direction)
			.endClass()
			.deriveClass <PointLight, Light>("PointLight")
				.addConstructor <void(*) (const Color &, const Vector3f &), RefCountedPtr <PointLight>>()
				.addData("position", &PointLight::position)
			.endClass()
			.deriveClass <SpotLight, Light>("SpotLight")
				.addConstructor <void(*) (const Color &, const Vector3f &, const Vector3f &, float, float), RefCountedPtr <SpotLight>>()
				.addData("position", &SpotLight::position)
				.addProperty("direction", &SpotLight::getDir, &SpotLight::setDir)
				.addData("inner", &SpotLight::inner)
				.addData("outer", &SpotLight::outer)
			.endClass()
			.beginClass <Scene>("Scene")
				.addFunction("getCamera", &Scene::getCameraRef)
				.addProperty("camera", &Scene::getCamera, &Scene::setCamera)
				.addFunction("getObject", &Scene::getObject)
				.addFunction("addObject", &Scene::addObject)
				.addFunction("deleteObject", &Scene::deleteObject)
				.addFunction("objectsSize", &Scene::objectsSize)
				.addFunction("getLight", &Scene::getLight)
				.addFunction("addLight", &Scene::addLight)
				.addFunction("deleteLight", &Scene::deleteLight)
				.addFunction("lightsSize", &Scene::lightsSize)
			.endClass()
		.endNamespace();
}

void setSceneSettings(Scene &scene, const cxxopts::ParseResult &opts)
{
	if (opts.count("super"))
	{
		scene.settings.insert({ Scene::Supersampling, opts["super"].as <size_t>() });
		scene.setProperty(Scene::SupersamplingJitter);
	}

	if (opts.count("dof"))
	{
		scene.settings.insert({ Scene::DOF, opts["dof"].as <size_t>() });
		scene.setProperty(Scene::DOF);
	}
}

void renderSingle(const string &filename, cxxopts::ParseResult &opts)
{
	Scene scene;
	setSceneSettings(scene, opts);
	auto start = chrono::steady_clock::now();

	if (!scene.loadScene(filename))
	{
		cerr << "File does not exist or malformed\n";
		return;
	}
	auto loadEnd = chrono::steady_clock::now();
#ifdef ASYNC_RENDER
	auto data = scene.renderParallel();
#else
	auto data = scene.render();
#endif
	auto renderEnd = chrono::steady_clock::now();

	writeImage(data, scene.getOutputFile());
	auto writeEnd = chrono::steady_clock::now();

	auto elapsedLoad = chrono::duration_cast <chrono::milliseconds>(loadEnd - start).count();
	auto elapsedRender = chrono::duration_cast <chrono::milliseconds>(renderEnd - loadEnd).count();
	auto elapsedWrite = chrono::duration_cast <chrono::milliseconds>(writeEnd - renderEnd).count();

	cerr << "Elapsed:"
		<< "\nLoad: " << elapsedLoad / 1000.f
		<< "\nRender: " << elapsedRender / 1000.f
		<< "\nWrite: " << elapsedWrite / 1000.f 
		<< endl;

	cerr << "Result saved to " << scene.getOutputFile() << endl;
}

void renderMultiple(const string &filename, const string &script, const cxxopts::ParseResult &opts)
{
	Scene scene;
	setSceneSettings(scene, opts);
	scene.loadScene(filename);

	lua_State *L = initScript(script);
	if (!L)
		return;
	loadScriptingAPI(L);
	ADD_LUA_VAR(L, "tracer", scene);
	if (!runScript(L))
		return;

	LuaRef scriptTick = getGlobal(L, "tick");

	long long totalTime = 0;
	long long execTime = 0;
	long long renderTime = 0;

#ifdef ASYNC_WRITE
	ctpl::thread_pool pool(1);
	vector <future <unsigned int>> futures;
#endif

	unique_ptr <FILE, decltype(&PCLOSE)> pipe(nullptr, PCLOSE);
	if (opts.count("anim") && opts.count("no-ffmpeg") == 0)
	{
		// Open pipe to ffmpeg
		string ffmpeg = opts["ffmpeg"].as <string>();
		ffmpeg += "ffmpeg -y -framerate ";
		ffmpeg += to_string(opts["framerate"].as <size_t>());
		ffmpeg += " -f image2pipe -i - ";
		ffmpeg += getOutputVideoName(scene.getOutputFile());
		
#ifdef _MSC_VER
		ffmpeg += " 2> nul";
#else
		ffmpeg += " 2> /dev/null";
#endif
		pipe = unique_ptr <FILE, decltype(&PCLOSE)>(POPEN(ffmpeg.c_str()), PCLOSE);
		
		if (!pipe.get())
		{
			cerr << "Could not init pipe\n";
			return;
		}
	}

	if (opts.count("save-frames") || (opts.count("anim") && opts.count("no-ffmpeg")))
	{
		// Create folder temp
		mkdir("temp");
	}


	vector <vector <Color>> blurRes;

	string out("temp/img");
	size_t skip = opts["skip"].as <size_t>();
	size_t framerate = opts["framerate"].as <size_t>();
	size_t frames = opts["frames"].as <size_t>() + skip;
	auto start = chrono::steady_clock::now();

	// Skip frames
	for (size_t i = 0; i < skip; ++i)
	{
		if (callLuaFunction(scriptTick, 1.f / framerate))
			break;
	}

	for (size_t i = skip; i < frames; ++i)
	{
		cerr << i + 1 << '/' << frames << "  ";
		auto start = chrono::steady_clock::now();
		
		if (callLuaFunction(scriptTick, 1.f / framerate))
			break;

		auto scriptEnd = chrono::steady_clock::now();
#ifdef ASYNC_RENDER
		auto data = scene.renderParallel();
#else
		auto data = scene.render();
#endif
		auto renderEnd = chrono::steady_clock::now();

		if (opts.count("save-frames") || (opts.count("anim") && opts.count("no-ffmpeg")))
		{
			// Save frame as image
			string num = to_string(i);
			string filename = out + string(4 - num.size(), '0') + num + ".png";
#ifdef ASYNC_WRITE
			futures.push_back(pool.push([](int id, const vector <vector <Color>> &data, const string &filename)
			{
				return writeImage(data, filename);
			}, data, filename));
#else
			writeImage(data, filename);
#endif
		}

		if (opts.count("anim") && !opts.count("no-ffmpeg"))
		{
			// Write to pipe
#ifdef ASYNC_WRITE
			futures.push_back(pool.push([](int id, const vector <vector <Color>> data, FILE *pipe)
			{
				return writeToFile(data, pipe);
			}, data, pipe.get()));
#else
			writeToFile(data, pipe.get());
#endif	
		}

		// For some reasons thread pool does not free memory from already finished functions
		// until future.get() called, which may lead to enormous memory consumption when
		// we render a big amount of frames
		// To avoid this, we will "flush" IO every 10th frame
#ifdef ASYNC_WRITE
		if (i % 10 == 9)
		{
			for (auto &f : futures)
				f.get();

			futures.clear();
		}
#endif
		if (opts.count("blur"))
		{
			if (blurRes.size() == 0)
				blurRes = data;
			else
			{
				for (size_t x = 0; x < data.size(); ++x)
				{
					for (size_t y = 0; y < data[x].size(); ++y)
						blurRes[x][y] += data[x][y];
				}
			}
		}

		totalTime += chrono::duration_cast <chrono::milliseconds>(renderEnd - start).count();
		execTime += chrono::duration_cast <chrono::microseconds>(scriptEnd - start).count();
		long long frameRenderTime = chrono::duration_cast <chrono::milliseconds>(renderEnd - scriptEnd).count();
		renderTime += frameRenderTime;

		cerr << frameRenderTime / 1000.f << endl;
	}

#ifdef ASYNC_WRITE
	for (auto &f : futures)
		f.get();
#endif

	if (opts.count("blur"))
	{
		// Divide every pixel by number of frames
		for (size_t x = 0; x < blurRes.size(); ++x)
		{
			for (size_t y = 0; y < blurRes[x].size(); ++y)
			{
				blurRes[x][y] /= static_cast <float>(frames);
			}
		}

		writeImage(blurRes, scene.getOutputFile());
	}

	auto end = chrono::steady_clock::now();
	cerr << "Elapsed:"
		<< "\nSript: " << (execTime / 1000.f) << " ms"
		<< "\nRender: " << renderTime / 1000.f
		<< "\nTotal: " << chrono::duration_cast <chrono::milliseconds>(end - start).count() / 1000.f << endl;

	lua_gc(L, 0, 0);
}

int main(int argc, char **argv)
{
	//chdir("scenes");
	cxxopts::Options options("Raytracer", "");
	options.add_options()
		("i,input", "Input .xml file", cxxopts::value <string>())
		("dof", "DOF (arg - amount of additional rays from camera lense)", cxxopts::value <size_t>()->implicit_value("20"))
		("super", "Supersampling (divide every pixel in arg x arg subpixels)", cxxopts::value <size_t>()->implicit_value("2"))
		("b,blur", "Motion blur", cxxopts::value <string>())
		("a,anim", "Animation", cxxopts::value <string>())
		("framerate", "Framerate", cxxopts::value <size_t>()->default_value("30"))
		("frames", "Amount of frames", cxxopts::value <size_t>()->default_value("30"))
		("no-ffmpeg", "Disable output to .mp4 file")
		("save-frames", "Save frames in temp/")
		("skip", "Skip first 'arg' frames", cxxopts::value <size_t>()->default_value("0"))
		("ffmpeg", "Path to ffmpeg", cxxopts::value <string>()->default_value(
#ifdef _MSC_VER
			""
#else
			//getRelativePath(argv[0]) + "ffmpeg/"
			""
#endif
		))
		("h,help", "Show usage");

	try
	{
		auto res = options.parse(argc, argv);

		if (res.count("h"))
		{
			cerr << options.help();
			return 0;
		}

		if (!res.count("i"))
		{
			cerr << "Input file not provided\n";
			return 0;
		}

		if (res.count("anim"))
		{
			renderMultiple(res["i"].as <string>(), res["anim"].as <string>(), res);
		}
		else if (res.count("blur"))
		{
			renderMultiple(res["i"].as <string>(), res["blur"].as <string>(), res);
		}
		else
		{
			renderSingle(res["i"].as <string>(), res);
		}
	}
	catch (const cxxopts::OptionException &e)
	{
		cerr << e.what() << endl;
	}

#ifdef _MSC_VER
	//system("pause");
#endif
	return 0;
}
