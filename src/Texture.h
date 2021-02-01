#pragma once

#include <stdint.h>
#include <vector>

struct Texture {
    enum Filter { LINEAR, LINEAR_MIPMAP_LINEAR, NEAREST };
    enum Wrap { REPEAT, CLAMP };
    enum Format { RGBA, RGB, R };
    enum Target { TEXTURE_2D, TEXTURE_3D };
    enum Type { UNSIGNED_BYTE, FLOAT };
    Type type = UNSIGNED_BYTE;
    Filter filter = LINEAR;
    Wrap wrap = REPEAT;
    Format format = RGBA;
    std::vector<uint8_t> data;
    int size[3] = {0, 0, 0};
    Target target;
};
