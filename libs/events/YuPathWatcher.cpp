#include <YuEvents/YuPathWatcher.h>

#include <YuEvents/YuSemaphore.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuThread.h>

#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>

YuPathWatcher::YuPathWatcher(const std::string& path, bool recursive, bool directory):
    root(path),
    isRecursive(recursive),
    isDirectory(directory),
    canceled(false),
    waitingSemaphore(NULL)
{
}

YuPathWatcher::~YuPathWatcher() {
}

bool
YuPathWatcher::wait(YuSemaphore* sem) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    mutex.lock();
    if (canceled) {
        mutex.unlock();
        return true;
    }
    if (!events.empty()) {
        mutex.unlock();
        return false;
    }

    waitingSemaphore = sem;
    int wasCanceled = 0;
    bool posted = sem->wait(thread, task, &wasCanceled, sizeof(wasCanceled), &mutex);
    return !posted || wasCanceled || task->getCanceled();
}

bool
YuPathWatcher::next(YuPathWatchNativeEvent* out) {
    mutex.lock();
    if (events.empty()) {
        mutex.unlock();
        return false;
    }

    currentEvent = events.front();
    events.pop_front();
    out->path = currentEvent.path.c_str();
    out->flags = currentEvent.flags;
    mutex.unlock();
    return true;
}

void
YuPathWatcher::cancel(YuSemaphore* sem) {
    YuSemaphore* waiter = NULL;
    mutex.lock();
    canceled = true;
    if (waitingSemaphore == sem) {
        waiter = waitingSemaphore;
        waitingSemaphore = NULL;
    }
    mutex.unlock();

    if (waiter) {
        int wasCanceled = 1;
        waiter->post(&wasCanceled, sizeof(wasCanceled), true);
    }
}

std::string
YuPathWatcher::parentPath(const std::string& path) {
    std::string::size_type slash = path.rfind('/');
    if (slash == std::string::npos) {
        return ".";
    }
    if (slash == 0) {
        return "/";
    }
    return path.substr(0, slash);
}

bool
YuPathWatcher::accepts(const std::string& path, uint64_t flags) const {
    if ((flags & YuPathWatchMustScanSubdirectories) && !isDirectory) {
        return path == parentPath(root) || path == root;
    }
    if (!isDirectory) {
        return path == root;
    }
    if (path == root) {
        return true;
    }
    bool rootEndsInSlash = !root.empty() && root[root.size() - 1] == '/';
    std::string::size_type nameStart = rootEndsInSlash ? root.size() : root.size() + 1;
    if (path.size() <= root.size() || path.compare(0, root.size(), root) != 0 ||
        (!rootEndsInSlash && path[root.size()] != '/')) {
        return false;
    }
    if (isRecursive) {
        return true;
    }
    return path.find('/', nameStart) == std::string::npos;
}

void
YuPathWatcher::enqueue(const std::vector<YuQueuedPathWatchEvent>& batch) {
    YuSemaphore* waiter = NULL;
    mutex.lock();
    if (!canceled) {
        for (const YuQueuedPathWatchEvent& event : batch) {
            if (!accepts(event.path, event.flags)) {
                continue;
            }
            bool rescansFile = (event.flags & YuPathWatchMustScanSubdirectories) && !isDirectory;
            const std::string& path = rescansFile ? root : event.path;
            if (!events.empty() && events.back().path == path) {
                events.back().flags |= event.flags;
            } else {
                events.push_back({ path, event.flags });
            }
        }
        if (!events.empty() && waitingSemaphore) {
            waiter = waitingSemaphore;
            waitingSemaphore = NULL;
        }
    }
    mutex.unlock();

    if (waiter) {
        int wasCanceled = 0;
        waiter->post(&wasCanceled, sizeof(wasCanceled), false);
    }
}

// *************************************************************************************************

static std::string
yumiPathWatchNormalizedPath(const char* path) {
    char resolved[PATH_MAX];
    if (realpath(path, resolved)) {
        return resolved;
    }
    return path;
}

extern "C" void*
yumiPathWatchCreate(const char* path, bool isRecursive) {
    if (!path) {
        return NULL;
    }

    std::string normalized = yumiPathWatchNormalizedPath(path);
    struct stat info;
    if (lstat(normalized.c_str(), &info) != 0) {
        return NULL;
    }

    YuPathWatcher* watcher = yumiCreatePathWatcher(normalized, isRecursive, S_ISDIR(info.st_mode));
    if (!watcher->start()) {
        delete watcher;
        return NULL;
    }
    return watcher;
}

extern "C" bool
yumiSuspendActiveTaskUntilPathWatchEvent(void* value, YuSemaphore* sem) {
    if (!value || !sem) {
        return true;
    }
    return static_cast<YuPathWatcher*>(value)->wait(sem);
}

extern "C" bool
yumiPathWatchNextEvent(void* value, YuPathWatchNativeEvent* out) {
    if (!value || !out) {
        return false;
    }
    return static_cast<YuPathWatcher*>(value)->next(out);
}

extern "C" void
yumiCancelPathWatchEventListener(void* value, YuSemaphore* sem) {
    if (value) {
        static_cast<YuPathWatcher*>(value)->cancel(sem);
    }
}

extern "C" void
yumiPathWatchRelease(void* value) {
    delete static_cast<YuPathWatcher*>(value);
}
