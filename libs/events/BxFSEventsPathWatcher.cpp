#include <BxEvents/BxPathWatcher.h>

#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>

#include <dirent.h>
#include <map>
#include <set>
#include <sys/stat.h>

static_assert(BxPathWatchMustScanSubdirectories == kFSEventStreamEventFlagMustScanSubDirs, "");
static_assert(BxPathWatchKernelDropped == kFSEventStreamEventFlagKernelDropped, "");
static_assert(BxPathWatchRootChanged == kFSEventStreamEventFlagRootChanged, "");
static_assert(BxPathWatchUnmounted == kFSEventStreamEventFlagUnmount, "");
static_assert(BxPathWatchCreated == kFSEventStreamEventFlagItemCreated, "");
static_assert(BxPathWatchRemoved == kFSEventStreamEventFlagItemRemoved, "");
static_assert(BxPathWatchMetadataModified == kFSEventStreamEventFlagItemInodeMetaMod, "");
static_assert(BxPathWatchRenamed == kFSEventStreamEventFlagItemRenamed, "");
static_assert(BxPathWatchModified == kFSEventStreamEventFlagItemModified, "");
static_assert(BxPathWatchIsFile == kFSEventStreamEventFlagItemIsFile, "");
static_assert(BxPathWatchIsDirectory == kFSEventStreamEventFlagItemIsDir, "");
static_assert(BxPathWatchIsSymbolicLink == kFSEventStreamEventFlagItemIsSymlink, "");

struct BxPathWatchSnapshotEntry {
    uint64_t device;
    uint64_t inode;
    mode_t mode;
    timespec modified;
    off_t size;
};

static const size_t fallbackSnapshotLimit = 4096;

class BxFSEventsPathWatcher : public BxPathWatcher {
public:
    BxFSEventsPathWatcher(const std::string& path, bool recursive, bool directory):
        BxPathWatcher(path, recursive, directory),
        stream(NULL),
        queue(NULL),
        fallbackTimer(NULL)
    {
    }

    ~BxFSEventsPathWatcher() {
        stop();
    }

    bool start() {
        std::string watchPath = isDirectory ? root : parentPath(root);
        CFStringRef path = CFStringCreateWithCString(
            kCFAllocatorDefault, watchPath.c_str(), kCFStringEncodingUTF8);
        if (!path) {
            return false;
        }

        const void* pathValues[] = { path };
        CFArrayRef paths = CFArrayCreate(
            kCFAllocatorDefault, pathValues, 1, &kCFTypeArrayCallBacks);
        CFRelease(path);
        if (!paths) {
            return false;
        }

        FSEventStreamContext context = { 0, this, NULL, NULL, NULL };
        FSEventStreamCreateFlags createFlags =
            kFSEventStreamCreateFlagFileEvents |
            kFSEventStreamCreateFlagWatchRoot |
            kFSEventStreamCreateFlagNoDefer;
        stream = FSEventStreamCreate(
            kCFAllocatorDefault,
            pathWatchCallback,
            &context,
            paths,
            kFSEventStreamEventIdSinceNow,
            0.02,
            createFlags);
        CFRelease(paths);
        if (!stream) {
            return false;
        }

        queue = dispatch_queue_create("brix.path-watch", DISPATCH_QUEUE_SERIAL);
        if (!queue) {
            FSEventStreamRelease(stream);
            stream = NULL;
            return false;
        }

        FSEventStreamSetDispatchQueue(stream, queue);
        if (!FSEventStreamStart(stream)) {
            FSEventStreamInvalidate(stream);
            FSEventStreamRelease(stream);
            stream = NULL;
            return startFallback();
        }
        return true;
    }

private:
    static void pathWatchCallback(
        ConstFSEventStreamRef,
        void* context,
        size_t count,
        void* paths,
        const FSEventStreamEventFlags flags[],
        const FSEventStreamEventId[])
    {
        const char* const* names = static_cast<const char* const*>(paths);
        std::vector<BxQueuedPathWatchEvent> batch;
        batch.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            batch.push_back({ names[i], flags[i] });
        }
        static_cast<BxFSEventsPathWatcher*>(context)->enqueue(batch);
    }

    static void dispatchBarrier(void*) {
    }

    static void fallbackTimerCallback(void* context) {
        static_cast<BxFSEventsPathWatcher*>(context)->pollFallback();
    }

    static uint64_t typeFlags(mode_t mode) {
        if (S_ISDIR(mode)) {
            return kFSEventStreamEventFlagItemIsDir;
        }
        if (S_ISLNK(mode)) {
            return kFSEventStreamEventFlagItemIsSymlink;
        }
        return kFSEventStreamEventFlagItemIsFile;
    }

    static bool sameModifiedTime(const timespec& left, const timespec& right) {
        return left.tv_sec == right.tv_sec && left.tv_nsec == right.tv_nsec;
    }

    bool collectSnapshot(
        const std::string& path,
        bool descend,
        std::map<std::string, BxPathWatchSnapshotEntry>& snapshot) const
    {
        struct stat info;
        if (lstat(path.c_str(), &info) != 0) {
            return true;
        }
        if (snapshot.size() >= fallbackSnapshotLimit) {
            return false;
        }

        snapshot[path] = {
            static_cast<uint64_t>(info.st_dev),
            static_cast<uint64_t>(info.st_ino),
            info.st_mode,
            info.st_mtimespec,
            info.st_size
        };
        if (!descend || !S_ISDIR(info.st_mode)) {
            return true;
        }

        DIR* directory = opendir(path.c_str());
        if (!directory) {
            return true;
        }
        bool complete = true;
        while (struct dirent* entry = readdir(directory)) {
            std::string name(entry->d_name);
            if (name == "." || name == "..") {
                continue;
            }
            std::string child = path + "/" + name;
            if (!collectSnapshot(child, isRecursive, snapshot)) {
                complete = false;
                break;
            }
        }
        closedir(directory);
        return complete;
    }

    bool snapshot(std::map<std::string, BxPathWatchSnapshotEntry>& result) const {
        return collectSnapshot(root, isDirectory, result);
    }

    void releaseQueue() {
#if !OS_OBJECT_USE_OBJC
        dispatch_release(queue);
#endif
        queue = NULL;
    }

    bool startFallback() {
        if (!snapshot(fallbackSnapshot)) {
            fallbackSnapshot.clear();
            releaseQueue();
            return false;
        }
        fallbackTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
        if (!fallbackTimer) {
            releaseQueue();
            return false;
        }
        dispatch_set_context(fallbackTimer, this);
        dispatch_source_set_event_handler_f(fallbackTimer, fallbackTimerCallback);
        dispatch_source_set_timer(
            fallbackTimer,
            dispatch_time(DISPATCH_TIME_NOW, 20 * NSEC_PER_MSEC),
            20 * NSEC_PER_MSEC,
            5 * NSEC_PER_MSEC);
        dispatch_resume(fallbackTimer);
        return true;
    }

    void pollFallback() {
        std::map<std::string, BxPathWatchSnapshotEntry> next;
        if (!snapshot(next)) {
            enqueue({ {
                root,
                kFSEventStreamEventFlagMustScanSubDirs | kFSEventStreamEventFlagItemIsDir
            } });
            dispatch_source_cancel(fallbackTimer);
            return;
        }
        typedef std::pair<uint64_t, uint64_t> Identity;
        std::map<Identity, std::string> removedByIdentity;
        std::map<Identity, std::string> addedByIdentity;
        std::set<std::string> renamedPaths;
        std::vector<BxQueuedPathWatchEvent> batch;

        for (const auto& previous : fallbackSnapshot) {
            if (next.find(previous.first) == next.end()) {
                const BxPathWatchSnapshotEntry& entry = previous.second;
                removedByIdentity[{ entry.device, entry.inode }] = previous.first;
            }
        }
        for (const auto& current : next) {
            if (fallbackSnapshot.find(current.first) == fallbackSnapshot.end()) {
                const BxPathWatchSnapshotEntry& entry = current.second;
                addedByIdentity[{ entry.device, entry.inode }] = current.first;
            }
        }
        for (const auto& removed : removedByIdentity) {
            auto added = addedByIdentity.find(removed.first);
            if (added == addedByIdentity.end()) {
                continue;
            }
            const BxPathWatchSnapshotEntry& oldEntry = fallbackSnapshot[removed.second];
            const BxPathWatchSnapshotEntry& newEntry = next[added->second];
            batch.push_back({
                removed.second,
                kFSEventStreamEventFlagItemRenamed | typeFlags(oldEntry.mode)
            });
            batch.push_back({
                added->second,
                kFSEventStreamEventFlagItemRenamed | typeFlags(newEntry.mode)
            });
            renamedPaths.insert(removed.second);
            renamedPaths.insert(added->second);
        }
        for (const auto& previous : fallbackSnapshot) {
            auto current = next.find(previous.first);
            if (current == next.end()) {
                if (renamedPaths.find(previous.first) == renamedPaths.end()) {
                    batch.push_back({
                        previous.first,
                        kFSEventStreamEventFlagItemRemoved | typeFlags(previous.second.mode)
                    });
                }
                continue;
            }
            const BxPathWatchSnapshotEntry& before = previous.second;
            const BxPathWatchSnapshotEntry& after = current->second;
            if (before.device != after.device || before.inode != after.inode) {
                batch.push_back({
                    previous.first,
                    kFSEventStreamEventFlagItemRemoved | typeFlags(before.mode)
                });
                batch.push_back({
                    current->first,
                    kFSEventStreamEventFlagItemCreated | typeFlags(after.mode)
                });
            } else if (!sameModifiedTime(before.modified, after.modified) ||
                before.size != after.size) {
                batch.push_back({
                    current->first,
                    kFSEventStreamEventFlagItemModified | typeFlags(after.mode)
                });
            }
        }
        for (const auto& current : next) {
            if (fallbackSnapshot.find(current.first) == fallbackSnapshot.end() &&
                renamedPaths.find(current.first) == renamedPaths.end()) {
                batch.push_back({
                    current.first,
                    kFSEventStreamEventFlagItemCreated | typeFlags(current.second.mode)
                });
            }
        }
        fallbackSnapshot.swap(next);
        enqueue(batch);
    }

    void stop() {
        if (stream) {
            FSEventStreamStop(stream);
            FSEventStreamInvalidate(stream);
        }
        if (fallbackTimer) {
            dispatch_source_cancel(fallbackTimer);
        }
        if (queue) {
            dispatch_sync_f(queue, NULL, dispatchBarrier);
        }
        if (stream) {
            FSEventStreamRelease(stream);
            stream = NULL;
        }
        if (fallbackTimer) {
#if !OS_OBJECT_USE_OBJC
            dispatch_release(fallbackTimer);
#endif
            fallbackTimer = NULL;
        }
        if (queue) {
#if !OS_OBJECT_USE_OBJC
            dispatch_release(queue);
#endif
            queue = NULL;
        }
    }

    FSEventStreamRef stream;
    dispatch_queue_t queue;
    dispatch_source_t fallbackTimer;
    std::map<std::string, BxPathWatchSnapshotEntry> fallbackSnapshot;
};

BxPathWatcher*
brixCreatePathWatcher(const std::string& path, bool recursive, bool directory) {
    return new BxFSEventsPathWatcher(path, recursive, directory);
}
