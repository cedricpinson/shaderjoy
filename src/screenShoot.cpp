#include "screenShoot.h"
#include "Application.h"
#include <glad/glad.h>
#include <stdint.h>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#endif
#if defined(_MSC_VER)
#pragma warning(push)
#endif

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"

#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4388)
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// must be called after swap buffer
bool screenShoot(Application* app, const char* filename)
{
    const int width = app->width * app->pixelRatio;
    const int height = app->height * app->pixelRatio;
    std::vector<uint8_t> buffer(size_t(width * height * 3));

    int rowPack;
    glGetIntegerv(GL_PACK_ALIGNMENT, &rowPack);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
    glPixelStorei(GL_PACK_ALIGNMENT, rowPack);

    stbi_flip_vertically_on_write(1);                                                  // NOLINT
    int result = stbi_write_png(filename, width, height, 3, buffer.data(), width * 3); // NOLINT
    return result != 0;
}
