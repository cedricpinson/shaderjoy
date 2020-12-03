#include "watcher.h"
#include "Application.h"

#include <thread>

void pal_sleep(uint64_t value) { std::this_thread::sleep_for(std::chrono::microseconds(value)); }

bool readFile(const char* path, std::vector<char>& buffer)
{
    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("cant open file %s\n", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t size = size_t(ftell(file));
    fseek(file, 0, SEEK_SET);
    buffer.resize(size);
    fread(buffer.data(), 1, size, file);
    fclose(file);

    printf("read file %s (%zu bytes) successfully\n", path, buffer.size());
    return true;
}

void fileWatcherThread(Application* application)
{
    Watcher& watcher = application->watcher;

    for (auto&& file : application->files) {
        WatchFile watchFile;
        watchFile.path = file;
        watcher._files.emplace_back(watchFile);
    }

    struct stat st;
    while (application->running.load()) {
        // printf("checking files\n");
        for (size_t i = 0; i < watcher._files.size(); i++) {

            if (!watcher.fileChanged()) {

                WatchFile& watchFile = watcher._files[i];
                const char* path = watchFile.path.c_str();
                stat(path, &st);
                time_t date = st.st_mtime;

                if (date != watchFile.lastChange) {
                    watcher.lock();
                    watchFile.lastChange = date;

                    readFile(path, watchFile.data);
                    watcher._fileChanged = int(i);
                    watcher.unlock();
                }
            }
        }
        pal_sleep(500000);
    }
}
