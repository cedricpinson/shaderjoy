#pragma once
#include <glad/glad.h>

#include <string>
#include <vector>

struct ProgramDescription {

    struct Uniform {
        std::string name;
        GLenum type = 0;
        int location = -1;
        int size = 0;
        union Data {
            struct {
                float f[4];
            } f;
            struct {
                int i[4];
            } i;
            float data[4];
        };
        Data data;
    };

    struct Attribute {
        std::string name;
        int location = -1;
    };

    std::vector<Uniform> uniforms;
    std::vector<Attribute> attributes;
};
