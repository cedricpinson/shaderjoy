#include "watcher.h"
#include "Application.h"
#include "timer.h"

#include <stb/stb_image.h>

#include <string.h>
#include <thread>

bool readShaderFile(WatchFile& watchFile)
{
    FILE* file = fopen(watchFile.path.c_str(), "rb");
    if (!file) {
        printf("cant open file %s\n", watchFile.path.c_str());
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t size = size_t(ftell(file));
    fseek(file, 0, SEEK_SET);
    watchFile.data.resize(size);
    fread(watchFile.data.data(), 1, size, file);
    fclose(file);

    printf("read file %s (%zu bytes) successfully\n", watchFile.path.c_str(), watchFile.data.size());
    return true;
}

bool readTextureFile(WatchFile& watchFile)
{
    if (watchFile.texture.target == Texture::TEXTURE_2D) {

        FILE* file = fopen(watchFile.path.c_str(), "rb");
        if (!file) {
            printf("cant open file %s\n", watchFile.path.c_str());
            return false;
        }

#if 1
        int channel = 0;
        stbi_set_flip_vertically_on_load(true);
        auto data = stbi_load_from_file(file, &watchFile.texture.size[0], &watchFile.texture.size[1], &channel, 0);
        const size_t size = size_t(watchFile.texture.size[0]) * size_t(watchFile.texture.size[1]) * size_t(channel);
        watchFile.texture.data.resize(size_t(size));
        memcpy(watchFile.texture.data.data(), data, size);
        stbi_image_free(data);
        fclose(file);
#else
        static uint8_t textureData[4] = {255, 0, 255, 255};
        const int channel = 4;
        watchFile.texture.data.resize(4);
        memcpy(watchFile.texture.data.data(), textureData, 4);
        watchFile.texture.size[0] = 1;
        watchFile.texture.size[1] = 1;
#endif

        printf("read image %s %dx%d : %d (%zu bytes)\n", watchFile.path.c_str(), watchFile.texture.size[0],
               watchFile.texture.size[1], channel, watchFile.texture.data.size());
        switch (channel) {
        case 1:
            watchFile.texture.format = Texture::R;
            break;
        case 2:
            printf("images with format greysacle/alpha (2 channels)  are not supported");
            return false;
            break;
        case 3:
            watchFile.texture.format = Texture::RGB;
            break;
        case 4:
            watchFile.texture.format = Texture::RGBA;
            break;
        }
        watchFile.texture.type = Texture::UNSIGNED_BYTE;

    } else {
        printf("texture 3d not supported yet\n");
        return false;
    }
    return true;
}

void fileWatcherThread(Application* application)
{
    Watcher& watcher = application->watcher;

    struct stat st;
    while (application->running.load()) {
        for (size_t i = 0; i < watcher._files.size(); i++) {

            if (!watcher.fileChanged()) {

                WatchFile& watchFile = watcher._files[i];
                const char* path = watchFile.path.c_str();
                stat(path, &st);
                time_t date = st.st_mtime;

                if (date != watchFile.lastChange) {
                    watcher.lock();
                    watchFile.lastChange = date;

                    bool success = false;
                    switch (watchFile.type) {
                    case WatchFile::SHADER:
                        success = readShaderFile(watchFile);
                        break;
                    case WatchFile::TEXTURE0:
                    case WatchFile::TEXTURE1:
                    case WatchFile::TEXTURE2:
                    case WatchFile::TEXTURE3:
                        success = readTextureFile(watchFile);
                        break;
                    }
                    if (success) {
                        watcher._fileChanged = int(i);
                    }
                    watcher.unlock();
                }
            }
        }
        sleepInMS(500);
    }
}
