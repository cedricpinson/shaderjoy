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
