#pragma once

#include "watcher.h"
#include <atomic>

struct Application {
    std::atomic<bool> running;
    Watcher watcher;

    std::vector<uint8_t> fragmentShader;
};
