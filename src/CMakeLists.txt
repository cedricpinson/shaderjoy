set(SOURCES
    imguiFrame.cpp
    imguiLoader.cpp
    opengl.cpp
    programReport.cpp
    timer.cpp
    window.cpp
    watcher.cpp
    screenShoot.cpp
    shaderjoy.cpp
    stbImageImpl.cpp
    stbImageWriteImpl.cpp
)

include_directories(${GLFW_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/deps)

add_executable(shaderjoy ${SOURCES})
target_link_libraries(shaderjoy ${GLFW_LIBRARY} ${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
