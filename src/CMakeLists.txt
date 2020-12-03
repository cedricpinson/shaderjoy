set(SOURCES
    opengl.cpp
    programReport.cpp
    timer.cpp
    window.cpp
    watcher.cpp
    shaderjoy.cpp
)

include_directories(${GLFW_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/deps)

add_executable(shaderjoy ${SOURCES})
target_link_libraries(shaderjoy ${GLFW_LIBRARY} ${OPENGL_LIBRARIES})
