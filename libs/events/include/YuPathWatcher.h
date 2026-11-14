#ifndef YU_PATHWATCHER_H
#define YU_PATHWATCHER_H

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuTaskMutex.h>

#include <deque>
#include <stdint.h>
#include <string>
#include <vector>

// *************************************************************************************************

const uint64_t YuPathWatchMustScanSubdirectories = 0x00000001;
const uint64_t YuPathWatchKernelDropped = 0x00000004;
const uint64_t YuPathWatchRootChanged = 0x00000020;
const uint64_t YuPathWatchUnmounted = 0x00000080;
const uint64_t YuPathWatchCreated = 0x00000100;
const uint64_t YuPathWatchRemoved = 0x00000200;
const uint64_t YuPathWatchMetadataModified = 0x00000400;
const uint64_t YuPathWatchRenamed = 0x00000800;
const uint64_t YuPathWatchModified = 0x00001000;
const uint64_t YuPathWatchIsFile = 0x00010000;
const uint64_t YuPathWatchIsDirectory = 0x00020000;
const uint64_t YuPathWatchIsSymbolicLink = 0x00040000;

struct YuQueuedPathWatchEvent {
    std::string path;
    uint64_t flags;
};

// *************************************************************************************************

class YuPathWatcher {
public:
    YuPathWatcher(const std::string& path, bool recursive, bool directory);
    virtual ~YuPathWatcher();

    virtual bool start() = 0;

    bool wait(YuSemaphore* sem);
    bool next(YuPathWatchNativeEvent* out);
    void cancel(YuSemaphore* sem);

protected:
    static std::string parentPath(const std::string& path);

    void enqueue(const std::vector<YuQueuedPathWatchEvent>& batch);

    std::string root;
    bool isRecursive;
    bool isDirectory;

private:
    bool accepts(const std::string& path, uint64_t flags) const;

    bool canceled;
    YuTaskMutex mutex;
    YuSemaphore* waitingSemaphore;
    std::deque<YuQueuedPathWatchEvent> events;
    YuQueuedPathWatchEvent currentEvent;
};

YuPathWatcher* yumiCreatePathWatcher(const std::string& path, bool recursive, bool directory);

// *************************************************************************************************

#endif
