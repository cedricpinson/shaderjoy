cmake_minimum_required(VERSION 3.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if (NOT CMAKE_VERSION VERSION_LESS 3.12)
    cmake_policy(SET CMP0077 NEW) # Enables override of options from parent CMakeLists.txt
endif()

if (NOT CMAKE_VERSION VERSION_LESS 3.15)
    cmake_policy(SET CMP0091 NEW) # Enables use of MSVC_RUNTIME_LIBRARY
    cmake_policy(SET CMP0092 NEW) # Enables clean /W4 override for MSVC
endif()

project(shaderjoy VERSION 0.14 LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

find_package(GLFW3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

if(MSVC)
    add_compile_options(/W4 /WX /wd4146)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    add_definitions(-D_USE_MATH_DEFINES)
else()
  add_compile_options(-Wall -Werror -Wshadow -pedantic -Wparentheses -Wno-import -Wreturn-type -Wmissing-braces -Wunknown-pragmas -Wuninitialized -Wunused -Wextra -fno-exceptions -fno-rtti -Wno-gnu-anonymous-struct -Wsign-conversion)
  add_compile_options(-pthread)
endif()

if(APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    include_directories(SYSTEM /usr/local/include)
endif(APPLE)

include("${CMAKE_MODULE_PATH}/format.cmake")

add_subdirectory(src)

