#pragma once

#include "Texture.h"
#include <assert.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <vector>

struct WatchFile {
    enum Type {
        TEXTURE0 = 0, // NOLINT
        TEXTURE1 = 1,
        TEXTURE2 = 2,
        TEXTURE3 = 3,
        SHADER = 4
    };
    WatchFile() {}
    WatchFile(Type fileType, const std::string& filename)
        : type(fileType)
        , path(filename)
    {}
    Type type = SHADER;
    std::string path;

    // later we will probably have a union of different datatype
    Texture texture;

    time_t lastChange = 0;
    std::vector<uint8_t> data;
};

typedef std::vector<WatchFile> WatchFileList;

struct Watcher {
    WatchFileList _files;
    std::mutex _filesMutex;
    int _fileChanged = -1;

    void lock() { _filesMutex.lock(); }
    void unlock() { _filesMutex.unlock(); }
    bool fileChanged() const { return _fileChanged != -1; }
    void resetFileChanged() { _fileChanged = -1; }
    const std::string& getFilename() const { return _files[size_t(_fileChanged)].path; }
    const WatchFile& getChangedFile() const { return _files[size_t(_fileChanged)]; }
    const std::vector<uint8_t>& getData() const
    {
        assert(_fileChanged != -1 && "should not call getData if file is not changed");
        return _files[size_t(_fileChanged)].data;
    }
};

struct Application;
void fileWatcherThread(Application*);
