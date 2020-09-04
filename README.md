# mathjit

**[WIP]**

Mathematical expression JIT compiler


# Features

Supports double & complex numbers

Only supports basic arithmetic operations for now.


# Building

Tested with latest MVC, code should be portable.

Depends solely on [Boost.Spirit.X3](https://github.com/boostorg/spirit) and [asmjit](https://github.com/asmjit/asmjit) which are both available through vcpkg

Example of build with CMake & vcpkg

```
mkdir build
cmake -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=PATH_TO\vcpkg\scripts\buildsystems\vcpkg.cmake -B build .
cmake --build build --config Release
```