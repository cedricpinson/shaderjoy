# shaderjoy
Basically shaderjoy is shadertoy but to run natively instead of the webpage.

![](https://github.com/cedricpinson/shaderjoy/raw/main/resources/shaderjoy.gif)

I started to do the raytracing series  https://raytracing.github.io/ on shadertoy but was limited because it's in the browser. I wanted to edit my shader with emacs reloading on each changes and commit my progression in a git repository. So I started shaderjoy, it follows the same variable names iFrame to be compatible with shadertoy. Enjoy editing shader natively with your favorite editor. 

## Build
To build it you will need to install cmake and glfw

### Macos
```
brew install glfw cmake
git clone git@github.com:cedricpinson/shaderjoy.git
cd shaderjoy
mkdir build
cd build
cmake ../
make
src/shaderjoy yourFragment.glsl
```

### Linux
```
# to install clang-11 https://apt.llvm.org/
apt-get install clang-11 clang-format-11 clang-tidy-11 make libcurl4-gnutls-dev libglfw3-dev cmake
git clone git@github.com:cedricpinson/shaderjoy.git
cd shaderjoy
mkdir build
cd build
cmake ../
make
src/shaderjoy yourFragment.glsl
```

### Windows
use windows way of generating projects https://cmake.org/runningcmake/

## How to use it
```
# to edit your shader live
src/shaderjoy yourFragment.glsl

# to execute your shader and save the first frame, shaderjoy will exit just after
src/shaderjoy --save-frame yourFragment.glsl

```

## links
- sokol https://github.com/floooh/sokol
- glfw https://github.com/glfw/glfw
- glad https://glad.dav1d.de/
- glslViewer https://github.com/patriciogonzalezvivo/glslViewer
- imgui https://github.com/ocornut/imgui
- stb https://github.com/nothings/stb
