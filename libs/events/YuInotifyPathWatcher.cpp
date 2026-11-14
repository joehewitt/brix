#include <YuEvents/YuPathWatcher.h>

#include <dirent.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

static const uint32_t kYuInotifyMask =
    IN_CREATE | IN_DELETE | IN_MODIFY | IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO |
    IN_DELETE_SELF | IN_MOVE_SELF | IN_DONT_FOLLOW | IN_ONLYDIR | IN_EXCL_UNLINK;
static const size_t kYuInotifyBufferSize = 64 * 1024;

class YuInotifyPathWatcher : public YuPathWatcher {
public:
    YuInotifyPathWatcher(const std::string& path, bool recursive, bool directory):
        YuPathWatcher(path, recursive, directory),
        watchesTree(recursive && directory),
        inotify(-1),
        stopDescriptor(-1),
        running(false)
    {
    }

    ~YuInotifyPathWatcher() {
        stop();
    }

    bool start() {
        inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        stopDescriptor = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (inotify < 0 || stopDescriptor < 0) {
            return false;
        }
        if (!addWatch(isDirectory ? root : parentPath(root))) {
            return false;
        }
        if (watchesTree) {
            addSubdirectories(root, NULL);
        }
        running = pthread_create(&thread, NULL, threadMain, this) == 0;
        return running;
    }

private:
    static void* threadMain(void* context) {
        sigset_t set;
        sigfillset(&set);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        pthread_setname_np(pthread_self(), "yumi-path-watch");
        static_cast<YuInotifyPathWatcher*>(context)->run();
        return NULL;
    }

    static std::string childPath(const std::string& directory, const char* name) {
        return directory == "/" ? directory + name : directory + "/" + name;
    }

    static bool hasPrefix(const std::string& path, const std::string& prefix) {
        return path == prefix ||
            (path.size() > prefix.size() && path.compare(0, prefix.size(), prefix) == 0 &&
             path[prefix.size()] == '/');
    }

    static uint64_t entryType(const std::string& path) {
        struct stat info;
        if (lstat(path.c_str(), &info) != 0) {
            return YuPathWatchIsFile;
        }
        if (S_ISDIR(info.st_mode)) {
            return YuPathWatchIsDirectory;
        }
        if (S_ISLNK(info.st_mode)) {
            return YuPathWatchIsSymbolicLink;
        }
        return YuPathWatchIsFile;
    }

    bool addWatch(const std::string& path) {
        int descriptor = inotify_add_watch(inotify, path.c_str(), kYuInotifyMask);
        if (descriptor < 0) {
            return false;
        }
        directories[descriptor] = path;
        return true;
    }

    void addSubdirectories(const std::string& path, std::vector<YuQueuedPathWatchEvent>* created) {
        DIR* directory = opendir(path.c_str());
        if (!directory) {
            return;
        }
        while (struct dirent* entry = readdir(directory)) {
            std::string name(entry->d_name);
            if (name == "." || name == "..") {
                continue;
            }
            std::string child = childPath(path, entry->d_name);
            uint64_t type = entry->d_type == DT_DIR ? YuPathWatchIsDirectory :
                entry->d_type == DT_UNKNOWN ? entryType(child) :
                entry->d_type == DT_LNK ? YuPathWatchIsSymbolicLink : YuPathWatchIsFile;
            if (created) {
                created->push_back({ child, YuPathWatchCreated | type });
            }
            if (type == YuPathWatchIsDirectory && addWatch(child)) {
                addSubdirectories(child, created);
            }
        }
        closedir(directory);
    }

    void renameWatches(const std::string& from, const std::string& to) {
        for (auto& entry : directories) {
            if (hasPrefix(entry.second, from)) {
                entry.second = to + entry.second.substr(from.size());
            }
        }
    }

    void removeWatches(const std::string& path) {
        for (auto entry = directories.begin(); entry != directories.end();) {
            if (hasPrefix(entry->second, path)) {
                inotify_rm_watch(inotify, entry->first);
                entry = directories.erase(entry);
            } else {
                ++entry;
            }
        }
    }

    void run() {
        alignas(struct inotify_event) char buffer[kYuInotifyBufferSize];
        struct pollfd descriptors[2] = { { inotify, POLLIN, 0 }, { stopDescriptor, POLLIN, 0 } };
        while (true) {
            if (poll(descriptors, 2, -1) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return;
            }
            if (descriptors[1].revents) {
                return;
            }
            std::vector<YuQueuedPathWatchEvent> batch;
            readEvents(buffer, batch);
            if (!batch.empty()) {
                enqueue(batch);
            }
        }
    }

    void readEvents(char* buffer, std::vector<YuQueuedPathWatchEvent>& batch) {
        std::unordered_map<uint32_t, std::string> movedAway;
        while (true) {
            ssize_t length = read(inotify, buffer, kYuInotifyBufferSize);
            if (length < 0 && errno == EINTR) {
                continue;
            }
            if (length <= 0) {
                break;
            }
            for (char* cursor = buffer; cursor < buffer + length;) {
                const struct inotify_event* event = reinterpret_cast<struct inotify_event*>(cursor);
                cursor += sizeof(struct inotify_event) + event->len;
                translate(event, movedAway, batch);
            }
        }
        for (const auto& moved : movedAway) {
            removeWatches(moved.second);
        }
    }

    void translate(
        const struct inotify_event* event,
        std::unordered_map<uint32_t, std::string>& movedAway,
        std::vector<YuQueuedPathWatchEvent>& batch)
    {
        if (event->mask & IN_Q_OVERFLOW) {
            if (watchesTree) {
                addSubdirectories(root, NULL);
            }
            uint64_t type = isDirectory ? YuPathWatchIsDirectory : 0;
            batch.push_back({ root, YuPathWatchMustScanSubdirectories | YuPathWatchKernelDropped | type });
            return;
        }
        auto found = directories.find(event->wd);
        if (found == directories.end()) {
            return;
        }
        std::string directory = found->second;
        if (event->mask & IN_IGNORED) {
            directories.erase(found);
            return;
        }
        if (event->mask & IN_UNMOUNT) {
            batch.push_back({ directory, YuPathWatchUnmounted | YuPathWatchIsDirectory });
            return;
        }
        if (event->len == 0) {
            if (!isDirectory || directory != root) {
                return;
            }
            if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
                batch.push_back({ root, YuPathWatchRootChanged });
            } else if (event->mask & IN_ATTRIB) {
                batch.push_back({ root, YuPathWatchMetadataModified | YuPathWatchIsDirectory });
            }
            return;
        }

        std::string path = childPath(directory, event->name);
        bool isDirectoryEntry = event->mask & IN_ISDIR;
        uint64_t type = isDirectoryEntry ? YuPathWatchIsDirectory : entryType(path);
        if (event->mask & IN_CREATE) {
            batch.push_back({ path, YuPathWatchCreated | type });
            if (isDirectoryEntry && watchesTree && addWatch(path)) {
                addSubdirectories(path, &batch);
            }
        }
        if (event->mask & IN_DELETE) {
            batch.push_back({ path, YuPathWatchRemoved | type });
        }
        if (event->mask & IN_MODIFY) {
            batch.push_back({ path, YuPathWatchModified | type });
        }
        if (event->mask & IN_ATTRIB) {
            batch.push_back({ path, YuPathWatchMetadataModified | type });
        }
        if (event->mask & IN_MOVED_FROM) {
            batch.push_back({ path, YuPathWatchRenamed | type });
            if (isDirectoryEntry && watchesTree) {
                movedAway[event->cookie] = path;
            }
        }
        if (event->mask & IN_MOVED_TO) {
            batch.push_back({ path, YuPathWatchRenamed | type });
            if (isDirectoryEntry && watchesTree) {
                auto moved = movedAway.find(event->cookie);
                if (moved != movedAway.end()) {
                    renameWatches(moved->second, path);
                    movedAway.erase(moved);
                } else if (addWatch(path)) {
                    addSubdirectories(path, NULL);
                }
            }
        }
    }

    void stop() {
        if (running) {
            uint64_t value = 1;
            ssize_t written = write(stopDescriptor, &value, sizeof(value));
            (void)written;
            pthread_join(thread, NULL);
            running = false;
        }
        if (stopDescriptor >= 0) {
            close(stopDescriptor);
            stopDescriptor = -1;
        }
        if (inotify >= 0) {
            close(inotify);
            inotify = -1;
        }
    }

    bool watchesTree;
    int inotify;
    int stopDescriptor;
    bool running;
    pthread_t thread;
    std::unordered_map<int, std::string> directories;
};

YuPathWatcher*
yumiCreatePathWatcher(const std::string& path, bool recursive, bool directory) {
    return new YuInotifyPathWatcher(path, recursive, directory);
}
