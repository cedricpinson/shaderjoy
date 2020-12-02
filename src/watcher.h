#pragma once

#include <assert.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <vector>

struct WatchFile {
    std::string path;
    time_t lastChange = 0;
    std::vector<char> data;
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
    const std::vector<char>& getData() const
    {
        assert(_fileChanged != -1 && "should not call getData if file is not changed");
        return _files[size_t(_fileChanged)].data;
    }
};

struct Application;
void fileWatcherThread(Application*);
