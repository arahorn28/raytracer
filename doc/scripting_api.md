# Scripting API

## Scene
Represented by exactly one object 'scene', which contains
all information about objects, lights and camera in scene
	
**Properties:**
```
Camera camera
```
**Methods:**
```
Object getObject(int)
void addObject(Object)
bool deleteObject(Object)
int objectsSize()

Light getLight(int)
void addLight(Light)
bool deleteLight(Light)
int lightsSize()
```

## Camera
**Properties:**
```
Vec position
Vec up
Vec lookat
float FOV (in radians)
float aperture
```
**Methods:**
```
void lookAt(Vec pos, Vec lookat, Vec up)
```

## Light
**Properties:**
```
bool on
Color color
```

## ParallelLight: Light
**Properties:**
```
Vec direction
```
**Methods:**
```
ParallelLight(Color color, Vec direction)
```

## PointLight: Light
**Properties:**
```
Vec position
```
**Methods:**
```
PointLight(Color color, Vec position)
```

## SpotLight: Light
**Properties:**
```
Vec position
Vec direction
float inner
float outer
```
**Methods:**
```
SpotLight(Color color, Vec pos, Vec dir, float inner, float outer)
```

## Object
**Properties:**
```
Mat transform
Material material
```

## Sphere: Object
**Properties:**
```
float r
```
**Methods:**
```
Sphere(float radius, Material mat, Mat transform, Mat inverseTransform)
```

## Mesh: Object
**Methods:**
```
Mesh(string pathToObj, Material mat, Mat transform, Mat invTransform)
```

## Material
**Properties:**
```
float ka
float kd
float ks
float exp
float reflectance
float transmittance
float iof
```

## MaterialSolid: Material
**Properties:**
```
Color color
```
**Methods:**
```
MaterialSolid(Color color, ...float x7...)
```

## MaterialTextured: Material
**Methods:**
```
MaterialSolid(string pathToTexture, ...float x7...)
```

## Mat
4x4 matrix
  
**Methods:**
```
Mat() - creates identity matrix
Mat operator * (Mat rhs)
Vec mul(Vec)
```
**Static methods:**
```
Mat fromTranslation(Vec)
Mat fromScaling(Vec)
Mat fromRotationX(float)
Mat fromRotationY(float)
Mat fromRotationZ(float)
```

## Vec
Vector of size 3
  
**Methods:**
```
Vec(float, float, float)
Vec operator +(Vec)
Vec operator -(Vec)
Vec operator *(float)
float dot(Vec)
float operator [](int) - read-only
void normalize()
```

## Color: Vec
**Methods:**
```
Color(float, float, float)
Color operator *(Color)
```
