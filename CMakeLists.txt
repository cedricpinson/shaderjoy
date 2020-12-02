cmake_minimum_required(VERSION 3.0)

if (NOT CMAKE_VERSION VERSION_LESS 3.12)
    cmake_policy(SET CMP0077 NEW) # Enables override of options from parent CMakeLists.txt
endif()

if (NOT CMAKE_VERSION VERSION_LESS 3.15)
    cmake_policy(SET CMP0091 NEW) # Enables use of MSVC_RUNTIME_LIBRARY
    cmake_policy(SET CMP0092 NEW) # Enables clean /W4 override for MSVC
endif()

project(shaderjoy VERSION 0.14 LANGUAGES CXX)

find_package(GLFW3 REQUIRED)
find_package(OpenGL REQUIRED)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Werror -Wshadow -pedantic -Wparentheses -Wno-import -Wreturn-type -Wmissing-braces -Wunknown-pragmas -Wuninitialized -Wunused -Wextra -fno-exceptions -fno-rtti -Wno-gnu-anonymous-struct)
endif()

if(APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    include_directories(SYSTEM /usr/local/include)
endif(APPLE)

add_subdirectory(src)
