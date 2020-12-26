#include "window.h"
#include "Application.h"
#include "glad/glad.h"

#include <GLFW/glfw3.h>
#include <stdio.h>

const char* const title = "shaderjoy - Press space to pause";

Application* gApplication = nullptr;

void setViewport(int _width, int _height)
{
    gApplication->width = _width;
    gApplication->height = _height;
    const float pixelRatio = gApplication->pixelRatio;
    glViewport(0, 0, int(float(_width) * pixelRatio), int(float(_height) * pixelRatio));
    // printf("setViewport %d x %d\n", int(_width * pixelRatio), int(_height * pixelRatio));
    gApplication->requestFrame = true;
}

float getPixelRatio(GLFWwindow* window)
{
    int w, h;
    int fw, fh;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &fw, &fh);
    return float(fw) / float(w);
}

GLFWwindow* setupWindow(WindowStyle style, Application* app)
{
    gApplication = app;

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
        app->width = mode->width;
        app->height = mode->height;
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        window = glfwCreateWindow(app->width, app->height, title, monitor, NULL);
    } else {
        window = glfwCreateWindow(app->width, app->height, title, NULL, NULL);
    }

    if (!window) {
        glfwTerminate();
        printf("ABORT: GLFW create window failed\n");
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGL()) {
        glfwTerminate();
        fprintf(stderr, "Faileid to initialize glad");
        return nullptr;
    }

    printf("OpenGL %s\n", glGetString(GL_VERSION));
    printf("%s\n", glGetString(GL_RENDERER));
    printf("Shading language : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int w, int h) {
        (void)window;
        setViewport(w, h);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        (void)window;
        (void)scancode;
        (void)action;
        (void)mods;
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            gApplication->pause = !gApplication->pause;
            if (gApplication->pause) {
                glfwSetWindowTitle(window, "shaderjoy - PAUSED");
            } else {
                glfwSetWindowTitle(window, title);
            }
        }
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        (void)window;
        (void)mods;
        // we dont care about all buttons. We handle only button 0 and 1
        if (button > 1) {
            return;
        }

        gApplication->mouseButtonClicked[button] = action == GLFW_PRESS;
    });

    app->pixelRatio = getPixelRatio(window);
    if (app->pixelRatio > 1) {
        glfwSetWindowSize(window, app->width / app->pixelRatio, app->height / app->pixelRatio);
    } else {
        setViewport(app->width, app->height);
    }
    printf("window size %d x %d : pixel ratio %f\n", app->width, app->height, app->pixelRatio);

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
