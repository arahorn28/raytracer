# Input File Specification
### Scene Description
The input file begins with the header and a scene node, that specifies the output file, followed by the camera specifications. 
This is followed by the lights and surfaces. Each surface consists of a material, and zero or more transformations.
Transformations will be aplied to the surface in the order they appear in the file.

### Basic
```xml
<?xml version="1.0" standalone="no" ?>
<!DOCTYPE scene SYSTEM "scene.dtd">
```
Every file has to start with this header that specifies the xml version and the schema that should be used:
```xml
<scene output_file="myImage.png">
  <background_color r="1.0" g="0.0" b="0.0"/>
  <!-- add scene here -->
</scene>
```
The scene node is the main node in the scene file. Every other node has to be a child node of it.
The output is sent to the file specified by `output_file`, in this example to "myImage.png".
`background_color` sets the color of the background for when rays do not intersect any geometry.

### Camera
```xml
<camera>
  <position x="1.0" y="-2.0E-10" z="-3"/>
  <lookat x="1" y="2" z="3"/>
  <up x="1" y="2" z="3"/>
  <horizontal_fov angle="45"/>
  <resolution horizontal="1920" vertical="1080"/>
  <max_bounces n="100"/>
  <aperture r="1.0"/>
</camera>
```
The camera node has to appear once in every file in the scene node. It has to include the following information:  
#### `<position x="1.0" y="-2.0E-10" z="-3"/>`  
Location of the camera in world coordinates.  
#### `<lookat x="1" y="2" z="3"/>`  
The camera is looking at this point in space.
#### `<up x="1" y="2" z="3"/>`  
Vector that defines which direction is up.  
#### `<horizontal_fov angle="45"/>`  
Angle, in degrees, that specifies half of the angle from the left side of the screen to the right side of the screen.
#### `<resolution horizontal="1920" vertical="1080"/>`
Pixel resolution of the final output image.
#### `<max_bounces n="100"/>`
Maximum number of bounces for reflection and refraction.
#### `<aperture r="1.0"/>`
Radius of aperture. Optional parameter that is used only when depth of field is enabled.

### Light
```xml
<lights>
  <!-- add lights here -->
</lights>
```
All the lights in the scene should be grouped in the `lights` node. There are four different light types that can be used in the scene:
```xml
<ambient_light>
  <color r="1.0" g="0.2" b="0.3"/>
</ambient_light>
```
Defines an ambient light with `color (r,g,b)` - all objects are illuminated by this light. _Note: the world can have precisely one ambient light._
```xml
<parallel_light>
  <color r="0.1" g="0.2" b="0.3"/>
  <direction x="1" y="2" z="3"/>
</parallel_light>
```
Defines a parallel light with `color (r,g,b)` - much like the sun, these lights are infinitely far away, and only have a direction vector `direction (x,y,z)`.
```xml
<point_light>
  <color r="0.1" g="0.2" b="0.3"/>
  <position x="1" y="2" z="3"/>
</point_light>
```
Defines a point light with `color (r,g,b)` and `position (x,y,z)`.
```xml
<spot_light>y
  <color r="0.1" g="0.2" b="0.3"/>
  <position x="1" y="2" z="3"/>
  <direction x="1" y="2" z="3"/>
  <falloff alpha1="1" alpha2="3"/>
</spot_light>
```
Defines a spot light with `color(r,g,b)` and `position (x,y,z)` that points in `direction (x,y,z)`. 
`falloff` with `angle1` and `angle`2, both in degrees, specify how the light falls off. 
For any angle between zero and `angle1`, the light should be just like a point light. 
Between `angle1` and `angle2`, the light falls off. For angles greater than angle2, the light isn't there.

### Surface/Geometry
```xml
<surfaces>
  <!-- Add geometric primitives (spheres/meshes) -->
</surface>
```
All surfaces of these scene have to be grouped in the surface node. There are two different types of geometric primitives:
```xml
<sphere radius="123">
  <position x="1" y="2" z="3"/>
  <!-- Material -->
  <!-- Transform -->
</sphere>
```
This adds a sphere to the world centered at the given point (`position`) with the given `radius`.
```xml
<mesh name="duck.dae">
  <!-- Material -->
  <!-- Transform -->
</mesh>
```
Adds a triangle mesh specified using the OBJ file format.

### Material
```xml
<material_solid>
  <color r="0.1" g="0.2" b="0.3"/>
  <phong ka="1" kd="1" ks="1" exponent="1"/>
  <reflectance r="1.0"/>
  <transmittance t="1.0"/>
  <refraction iof="1.0"/>
</material_solid>
```
This defines a non-textured material
#### `<color r="0.1" g="0.2" b="0.3"/>`
Specifies the materials color.
#### `<phong ka="1" kd="1" ks="1" exponent="1"/>`
Specifies the coefficients for the phong illumination model. 
`ka` is the ambient component, `kd` is the diffuse component, `ks` is the specular component and `exponent` is the Phong cosine power for highlights.
#### `<reflectance r="1.0"/>`
Specifies the reflactance `r` (fraction of the contribution of the reflected ray).
#### `<transmittance t="1.0"/>`
Specifies  the transmittance `t` (fraction of contribution of the transmitting ray).
#### `<refraction iof="1"/>`
Specifies the index of refraction `iof`.  
Usually, 0 <= Ka <= 1, 0 <= Kd <= 1, and 0 <= Ks <= 1, though it is not required that Ka + Kd + Ks == 1. 
```xml
<material_textured>
  <texture name=""/>
  <phong ka="1" kd="1" ks="1" exponent="1"/>
  <reflectance r="1.0"/>
  <transmittance t="1.0"/>
  <refraction iof="1.0"/>
</material_textured>
```
This defines a textured material
#### `<texture name=""/>`
Specifies the texture file that should be used for this material.  
All other inputs are the same as the non-textured material.
### Transformation
```xml
<transforms>
  <translate x="1" y="1" z="1"/>
  <scale x="1" y="1" z="1"/>
  <rotateX theta="1"/>
  <rotateY theta="1"/>
  <rotateZ theta="1"/>
</transforms>
```
Transformations can be specified in every geometric primitive (sphere/mesh). 
There can be zero or more transformations that will be applied in the order of their appearence in the block.
#### `<translate x="1" y="1" z="1"/>`
Moves an object by the vector `[x,y,z]`.
#### `<scale x="1" y="1" z="1"/>`
Scales an object by `x` on the x-axis, `y` on the y-axis and `z` on the z-axis.  
#### `<rotateX theta="1"/>`  
#### `<rotateY theta="1"/>`  
#### `<rotateZ theta="1"/>`  
Rotates an object by `theta` degrees around the specific axis.
## Example
The following is an example of the expected input:
```xml
<?xml version="1.0" standalone="no" ?>
<!DOCTYPE scene SYSTEM "scene.dtd">

<scene output_file="myImage.png">
  <background_color r="1.0" g="0.0" b="0.0"/>
  <camera>
    <position x="1.0" y="-2.0E-10" z="-3"/>
    <lookat x="1" y="2" z="3"/>
    <up x="1" y="2" z="3"/>
    <horizontal_fov angle="90"/>
    <resolution horizontal="1920" vertical="1080"/>
    <max_bounces n="100"/>
  </camera>
  <lights>
    <ambient_light>
      <color r="0.1" g="0.2" b="0.3"/>
    </ambient_light>
    <point_light>
      <color r="0.1" g="0.2" b="0.3"/>
      <position x="1" y="2" z="3"/>
    </point_light>
    <parallel_light>
      <color r="0.1" g="0.2" b="0.3"/>
      <direction x="1" y="2" z="3"/>
    </parallel_light>
    <spot_light>
      <color r="0.1" g="0.2" b="0.3"/>
      <position x="1" y="2" z="3"/>
      <direction x="1" y="2" z="3"/>
      <falloff alpha1="1" alpha2="3"/>
    </spot_light>
  </lights>
  <surfaces>
    <sphere radius="123">
      <position x="1" y="2" z="3"/>
      <material_solid>
        <color r="0.1" g="0.2" b="0.3"/>
        <phong ka="1.0" kd="1.0" ks="1.0" exponent="1"/>
        <reflectance r="1.0"/>
        <transmittance t="1.0"/>
        <refraction iof="1.0"/>
      </material_solid>
      <transform>
        <translate x="1" y="1" z="1"/>
        <scale x="1" y="1" z="1"/>
        <rotateX theta="1"/>
        <rotateY theta="1"/>
        <rotateZ theta="1"/>
      </transform>
    </sphere>
    <mesh name="duck.dae">
      <material_textured>
        <texture name=""/>
        <phong ka="1.0" kd="1.0" ks="1.0" exponent="1"/>
        <reflectance r="1.0"/>
        <transmittance t="1.0"/>
        <refraction iof="1.0"/>
      </material_textured>
      <transform>
        <translate x="1" y="1" z="1"/>
        <scale x="1" y="1" z="1"/>
        <rotateX theta="1"/>
        <rotateY theta="1"/>
        <rotateZ theta="1"/>
        <translate x="1" y="1" z="1"/>
        <scale x="1" y="1" z="1"/>
      </transform>
    </mesh>
  </surfaces>
</scene>
```
