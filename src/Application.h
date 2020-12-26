#pragma once

#include "programReport.h"
#include "watcher.h"
#include <atomic>

struct Application {
    std::atomic<bool> running;
    Watcher watcher;
    float frameRate = 0.0f;
    int width = 1280;
    int height = 768;
    float pixelRatio = 0;
    std::vector<std::string> files;
    bool pause = false;
    bool requestFrame = true;
    bool mouseButtonClicked[2] = {false, false};
    ShaderCompileReport shaderReport;
};
