#pragma once

#include "watcher.h"
#include <atomic>

struct Application {
    std::atomic<bool> running;
    Watcher watcher;
    int width = 1280;
    int height = 768;
    float pixelRatio = 0;
    std::vector<std::string> files;
    bool pause = false;
    bool requestFrame = true;
};
