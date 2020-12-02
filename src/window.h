#pragma once

struct GLFWwindow;
enum WindowStyle { HEADLESS = 0, REGULAR, ALLWAYS_ON_TOP, FULLSCREEN };
struct Application;

GLFWwindow* setupWindow(WindowStyle style, Application* app);
void cleanupWindow(GLFWwindow* window);
