#pragma once

struct GLFWwindow;
enum WindowStyle { HEADLESS = 0, REGULAR, ALLWAYS_ON_TOP, FULLSCREEN };

GLFWwindow* setupWindow(WindowStyle style, int* width, int* height, float* pixelRatio);
void cleanupWindow(GLFWwindow* window);
