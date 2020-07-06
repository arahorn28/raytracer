# Raytracer
Simple raytracer that was created as a part of assignment for Computer Graphics class.

### Prerequisites
Raytracer uses ffmpeg to create animation. By default, it expects to find executable in PATH, but you can specify path to ffmpeg folder with `--ffmpeg` option. 
It's also possible to disable output to ffmpeg with `--no-ffmpeg`. Use `-h` to get detailed description of available options.

### Installation
```bash
cd raytracer
make
```

### Examples
I've provided a couple of examples to demonstrate different effects
```bash
cd examples
../raytracer -i dof.xml --dof=50
../raytracer -i blur1.xml -b scripts/blur1.lua
../raytracer -i blur2.xml -b scripts/blur2.lua
../raytracer -i motion.xml -a scripts/motion.lua --frames 480
```

### Features
Some of the implemented features:
* Light source
  * Ambient
  * Directional
  * Point
  * Spot
* Phong reflection model
* Depth of field
* Motion blur
* Animation (sripting with Lua)
* OBJ file support

### Animation
For animation I've implemented scripting support with Lua. In essence, raytracer
renders multiple images, which it can then combine in .mp4 file using ffmpeg
(default behaviour), save as images in `temp/` folder, or both.  
Motion blur is implemented using same scripting technique, but instead of outputing
to .mp4 file, all images will be combined in one image. Behaviour of objects
and camera should be described in script file; exposure and intervals
can be manipulated with options `--framerate` and `--frames`: 
`exposure time = frames / framerate`

### Scripting
Every script should contain function `tick(dt)` that would be called
every frame with `dt` = time interval between frames. This function can
optionally return bool value. In case it is equal to False, raytracer stops
execution. 
Scripting can be used not only for animation and motion blur, but
also as alternative for scene description for complicated scenes 
that are easier to create with scripts than manually with xml file.
```bash
./raytracer -i input.xml -b script.lua --frames 1
```
_Note: all classes and objects exposed to scripting engine are located in namespace `tracer`.
See files in `examples/scripts` for examples._
