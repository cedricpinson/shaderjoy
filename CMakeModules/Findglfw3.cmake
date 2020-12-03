# Try to find GLFW3 library and include dir.
# Once done this will define
#
# GLFW_FOUND
# GLFW_INCLUDE_DIR
# GLFW_LIBRARY

# The following variables can be set as arguments for the module.
# GLFW_DIR
# GLFW_ROOT

include(FindPackageHandleStandardArgs)

find_path(GLFW_INCLUDE_DIR
    NAMES
    GLFW/glfw3.h
    PATHS
    ${CMAKE_SOURCE_DIR}/deps/glfw/include
    ${CMAKE_SOURCE_DIR}/deps/glfw-3.2.1/include
    ${CMAKE_SOURCE_DIR}/vendors/glfw/include
    ${CMAKE_SOURCE_DIR}/vendors/glfw-3.2.1/include
    ${CMAKE_SOURCE_DIR}/../deps/glfw/include
    ${CMAKE_SOURCE_DIR}/../deps/glfw-3.2.1/include
    ${GLFW_DIR}/include
    ${GLFW_ROOT}/include
    $ENV{GLFW_DIR}/include
    $ENV{GLFW_ROOT}/include
    $ENV{PROGRAMFILES}/GLFW/include
    /usr/include
    /usr/local/include
    /sw/include
    /opt/local/include
    ~/Library/Frameworks
    /Library/Frameworks
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    NO_DEFAULT_PATH
    DOC "The directory where GLFW/glfw3.h resides" )

find_library( GLFW_LIBRARY
    NAMES
    glfw3dll glfw3 glfw
    PATHS
    ${CMAKE_SOURCE_DIR}/deps/glfw/lib
    ${CMAKE_SOURCE_DIR}/deps/glfw-3.2.1/lib
    ${CMAKE_SOURCE_DIR}/vendors/glfw/lib
    ${CMAKE_SOURCE_DIR}/vendors/glfw-3.2.1/lib
    ${GLFW_DIR}/lib
    ${GLFW_ROOT}/lib
    $ENV{GLFW_DIR}/lib
    $ENV{GLFW_ROOT}/lib
    $ENV{PROGRAMFILES}/GLFW/lib
    $ENV{PROGRAMFILES}/GLFW3/lib
    ${GLFW_INCLUDE_DIR}/../lib
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /sw/lib
    /opt/local/lib
    /usr/lib/x86_64-linux-gnu
    ~/Library/Frameworks
    /Library/Frameworks
    /opt/local/Library/Frameworks #macports
    NO_DEFAULT_PATH
    DOC "The GLFW library")

find_package_handle_standard_args(GLFW DEFAULT_MSG
    GLFW_INCLUDE_DIR
    GLFW_LIBRARY
    )

# If GLFW was not found try to search for deps/glfw to compile it
if (NOT GLFW_LIBRARY)
    # Check the source is there (by looking for the cmake file)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw/CMakeLists.txt")
        include_directories("deps/glfw/include")
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW lib only")
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only")
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
        set(GLFW_BUILD_INSTALL OFF CACHE BOOL "GLFW lib only")
        add_subdirectory(deps/glfw)
        set(GLFW_LIBRARY glfw)
    endif()
endif()

mark_as_advanced( GLFW_FOUND )
