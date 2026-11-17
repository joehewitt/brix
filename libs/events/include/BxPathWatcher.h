#ifndef BX_PATHWATCHER_H
#define BX_PATHWATCHER_H

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxTaskMutex.h>

#include <deque>
#include <stdint.h>
#include <string>
#include <vector>

// *************************************************************************************************

const uint64_t BxPathWatchMustScanSubdirectories = 0x00000001;
const uint64_t BxPathWatchKernelDropped = 0x00000004;
const uint64_t BxPathWatchRootChanged = 0x00000020;
const uint64_t BxPathWatchUnmounted = 0x00000080;
const uint64_t BxPathWatchCreated = 0x00000100;
const uint64_t BxPathWatchRemoved = 0x00000200;
const uint64_t BxPathWatchMetadataModified = 0x00000400;
const uint64_t BxPathWatchRenamed = 0x00000800;
const uint64_t BxPathWatchModified = 0x00001000;
const uint64_t BxPathWatchIsFile = 0x00010000;
const uint64_t BxPathWatchIsDirectory = 0x00020000;
const uint64_t BxPathWatchIsSymbolicLink = 0x00040000;

struct BxQueuedPathWatchEvent {
    std::string path;
    uint64_t flags;
};

// *************************************************************************************************

class BxPathWatcher {
public:
    BxPathWatcher(const std::string& path, bool recursive, bool directory);
    virtual ~BxPathWatcher();

    virtual bool start() = 0;

    bool wait(BxSemaphore* sem);
    bool next(BxPathWatchNativeEvent* out);
    void cancel(BxSemaphore* sem);

protected:
    static std::string parentPath(const std::string& path);

    void enqueue(const std::vector<BxQueuedPathWatchEvent>& batch);

    std::string root;
    bool isRecursive;
    bool isDirectory;

private:
    bool accepts(const std::string& path, uint64_t flags) const;

    bool canceled;
    BxTaskMutex mutex;
    BxSemaphore* waitingSemaphore;
    std::deque<BxQueuedPathWatchEvent> events;
    BxQueuedPathWatchEvent currentEvent;
};

BxPathWatcher* brixCreatePathWatcher(const std::string& path, bool recursive, bool directory);

// *************************************************************************************************

#endif
