#include "window.h"
#include "glad/glad.h"

#include <GLFW/glfw3.h>
#include <stdio.h>

const char* const title = "shaderjoy";

int* gWidth = nullptr;
int* gHeight = nullptr;
float* gPixelRatio = nullptr;

void setViewport(int _width, int _height)
{
    *gWidth = _width;
    *gHeight = _height;
    const float pixelRatio = *gPixelRatio;
    glViewport(0, 0, int(float(_width) * pixelRatio), int(float(_height) * pixelRatio));
    printf("setViewport %d x %d\n", int(_width * pixelRatio), int(_height * pixelRatio));
}

float getPixelRatio(GLFWwindow* window)
{
    int w, h;
    int fw, fh;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &fw, &fh);
    return float(fw) / float(w);
}

GLFWwindow* setupWindow(WindowStyle style, int* width, int* height, float* pixelRatio)
{

    gWidth = width;
    gHeight = height;
    gPixelRatio = pixelRatio;

// ok on macos if GLFW_COCOA_CHDIR_RESOURCES is not set to false
// then it's not possible to run an executables with the name sketchfab/sketchfab
// glfw would try to set a different chdir and it will fails from the command line invocation like this:
#ifdef __APPLE__
    glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif

    if (!glfwInit()) {
        printf("Failed to initialize GLFW");
        return nullptr;
    }

    GLFWwindow* window = nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_SAMPLES, 0);
    // glfwWindowHint(GLFW_ALPHA_BITS, 8);

    if (style == HEADLESS)
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    else if (style == ALLWAYS_ON_TOP)
        glfwWindowHint(GLFW_FLOATING, GL_TRUE);

    if (style == FULLSCREEN) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        *gWidth = mode->width;
        *gHeight = mode->height;
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        window = glfwCreateWindow(*gWidth, *gHeight, title, monitor, NULL);
    } else {
        window = glfwCreateWindow(*gWidth, *gHeight, title, NULL, NULL);
    }

    if (!window) {
        glfwTerminate();
        printf("ABORT: GLFW create window failed\n");
        return nullptr;
    }

    // glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwMakeContextCurrent(window);
    *gPixelRatio = getPixelRatio(window);
    printf("window size %d x %d : pixel ratio %f\n", *gWidth, *gHeight, *gPixelRatio);

    if (!gladLoadGL()) {
        glfwTerminate();
        fprintf(stderr, "Faileid to initialize glad");
        return nullptr;
    }

    printf("OpenGL %s\n", glGetString(GL_VERSION));
    printf("%s\n", glGetString(GL_RENDERER));
    printf("Shading language : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glfwSetWindowSizeCallback(window, [](GLFWwindow* _window, int _w, int _h) {
        (void)_window;
        setViewport(_w, _h);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* _window, int _key, int _scancode, int _action, int _mods) {
        (void)_window;
        (void)_scancode;
        (void)_action;
        (void)_mods;
        if (_key == GLFW_KEY_ESCAPE && _action == GLFW_PRESS) {
            glfwSetWindowShouldClose(_window, GLFW_TRUE);
        }
    });

    setViewport(*gWidth, *gHeight);

    glfwSwapInterval(1);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    return window;
}

void cleanupWindow(GLFWwindow* window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}
