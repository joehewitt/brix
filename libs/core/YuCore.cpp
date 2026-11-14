
#include "YuCore/YuCore.h"

#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <math.h>
#include <limits.h>
#include <cfloat>
#include <cerrno>

#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#include <libproc.h>
#endif
#else
#include <dirent.h>
#endif
#include <termios.h>
#include <fcntl.h>

#include <sys/dir.h>
#include <sys/stat.h>

// #include <gc.h>

#include <sys/types.h>
#include <sys/time.h>

#include "lookup3.h"

// **********************************************************************************************

static uint64_t allocTotal = 0;

#if defined(__aarch64__)
extern "C" __attribute__((naked)) void yumiZeroStackFrame(void*, size_t) {
    __asm__ volatile(
        "1:\n"
        "cmp x1, #16\n"
        "b.lo 2f\n"
        "stp xzr, xzr, [x0], #16\n"
        "sub x1, x1, #16\n"
        "b 1b\n"
        "2:\n"
        "tbz x1, #3, 3f\n"
        "str xzr, [x0], #8\n"
        "3:\n"
        "tbz x1, #2, 4f\n"
        "str wzr, [x0], #4\n"
        "4:\n"
        "tbz x1, #1, 5f\n"
        "strh wzr, [x0], #2\n"
        "5:\n"
        "tbz x1, #0, 6f\n"
        "strb wzr, [x0]\n"
        "6:\n"
        "ret\n");
}
#else
extern "C" void yumiZeroStackFrame(void*, size_t) {
}
#endif

void* yumiMalloc(size_t size) {
    // printf("malloc %d\n", size);
    // allocTotal += size;
    void* ret = malloc(size ? size : 1);
    // void* ret = GC_MALLOC(size);
    if (!ret) {
        fprintf(stderr, "Malloc fail\n");
        exit(1);
    }
    return ret;
}

void* yumiMallocAtomic(size_t size) {
    // printf("malloc %d\n", size);
    // allocTotal += size;
    void* ret = malloc(size ? size : 1);
    // void* ret = GC_MALLOC_ATOMIC(size);
    if (!ret) {
        fprintf(stderr, "Malloc fail\n");
        exit(1);
    }
    return ret;
}

void* yumiRealloc(void* buf, size_t size) {
    // printf("realloc %d\n", size);
    // allocTotal += size;

    void* ret = realloc(buf, size ? size : 1);
    // void* ret = GC_REALLOC(buf, size);
    if (!ret) {
        fprintf(stderr, "Malloc fail\n");
        exit(1);
    }
    return (void*)ret;
}

// **********************************************************************************************

static int savedArgC = 0;
static char** savedArgV = NULL;
static bool didInitProcess = false;

static void
yumiInitProcess() {
    if (didInitProcess) {
        return;
    }

    signal(SIGPIPE, SIG_IGN);
    didInitProcess = true;
}

__attribute__((constructor))
static void
yumiInitProcessAtLoad() {
    yumiInitProcess();
}

extern "C" bool
yumiHostIsLittleEndian() {
    const uint16_t probe = 1;
    return *reinterpret_cast<const uint8_t*>(&probe) == 1;
}

extern "C" void
yumiSaveArgs(int argc, char** argv) {
    yumiInitProcess();
    savedArgC = argc;
    savedArgV = argv;
#if YU_ENABLE_BENCH_METRICS
    const char* metricsPath = std::getenv("YUMI_BENCH_METRICS_PATH");
    if (metricsPath && *metricsPath) {
        yumiBenchInitMetrics();
        std::atexit(yumiBenchMaybeWriteMetrics);
    }
#endif
}

extern "C" int
yumiGetArgC() {
    return savedArgC;

}

extern "C" char**
yumiGetArgV() {
    return savedArgV;
}

// *************************************************************************************************

// static int alive = 0;

// static void
// yumiFinalizer(void* obj, void* clientData) {
//     alive -= 1;
//     printf("alive %d of %d\n", alive, (size_t)clientData);
// }

static constexpr unsigned int kUIDBlockSize = 1024;

static std::atomic<unsigned int> globalUID {1000};

struct UIDBlock {
    unsigned int last;
    unsigned int limit;
};

static thread_local UIDBlock threadUIDBlock = {0, 0};

extern "C" unsigned int
yumiGenerateUID() {
    UIDBlock& block = threadUIDBlock;
    if (block.last == block.limit) {
        block.last = globalUID.fetch_add(kUIDBlockSize, std::memory_order_relaxed);
        block.limit = block.last + kUIDBlockSize;
    }
    return ++block.last;
}

// *************************************************************************************************

extern "C" char*
yumiArrayCopySlice(char* self, const void* source, size_t size) {
    if (size == 0) return self;
    return (char*)memmove(self, source, size);
}

extern "C" char*
yumiArrayCopySliceDescribed(char* self, const void* source, size_t size, void* descriptor) {
    if (size == 0) return self;
    yumiBulkWriteBarrierFast(self, (char*)source, size, descriptor);
    return (char*)memmove(self, source, size);
}

extern "C" char*
yumiArraySlice(char* self, size_t start, size_t end) {
    size_t size = end - start;
    char* slice = (char*)YU_MALLOC(size);
    memcpy(slice, self + start, size);
    return slice;
}

// *************************************************************************************************

static int
yumiSetTerminalAttrOnFD(int fd, int action, const struct termios* state) {
    int flags = fcntl(fd, F_GETFL, 0);
    bool restoreNonBlocking = flags >= 0 && (flags & O_NONBLOCK);

    if (restoreNonBlocking && fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) < 0) {
        return -1;
    }

    int result = tcsetattr(fd, action, state);
    int savedErrno = errno;

    if (restoreNonBlocking) {
        fcntl(fd, F_SETFL, flags);
    }

    if (result < 0) {
        errno = savedErrno;
        return -1;
    }

    return 0;
}

static int
yumiSetTerminalAttr(int fd, int action, const struct termios* state) {
    if (yumiSetTerminalAttrOnFD(fd, action, state) == 0) {
        return 0;
    }

    int savedErrno = errno;
    if (savedErrno == EPERM && fd != STDIN_FILENO && isatty(STDIN_FILENO)) {
        if (yumiSetTerminalAttrOnFD(STDIN_FILENO, action, state) == 0) {
            return 0;
        }
    }

    errno = savedErrno;
    return -1;
}

extern "C" int
yumiTerminalEnter(int fd, struct termios* oldState) {
    struct termios newState;

    if (!isatty(fd)) {
        // setError("Stream is not a TTY");
        return -1;
    }

    if (tcgetattr(fd, oldState) == -1) {
        // setError("Unable to modify terminal");
        return -2;
    }

    // This code originates from:
    // https://github.com/antirez/linenoise/blob/master/linenoise.c

    newState = *oldState;

    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    newState.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON);

    /* output modes - disable post processing */
    // newState.c_oflag &= ~(OPOST);
    newState.c_oflag |= (ONLCR);

    /* control modes - set 8 bit chars */
    newState.c_cflag |= (CS8);

    /* local modes - echoing off, canonical off, no extended functions,*/
    newState.c_lflag &= ~(ECHO | ICANON | IEXTEN);

    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    newState.c_cc[VMIN] = 1;
    newState.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (yumiSetTerminalAttr(fd, TCSADRAIN, &newState) < 0) {
        // setError("Unable to modify terminal");
        return -3;
    }

    return 0;
}

extern "C" int
yumiTerminalExit(int fd, struct termios* state) {
    if (yumiSetTerminalAttr(fd, TCSAFLUSH, state) < 0) {
        return -1;
    }

    return 0;
}

extern "C" int
yumiTerminalSetSignals(int fd, int enabled) {
    struct termios state;
    if (tcgetattr(fd, &state) == -1) {
        return -1;
    }

    if (enabled) {
        state.c_lflag |= ISIG;
    } else {
        state.c_lflag &= ~ISIG;
    }

    if (yumiSetTerminalAttr(fd, TCSADRAIN, &state) < 0) {
        return -2;
    }

    return 0;
}

extern "C" bool
yumiReadDirEntry(DIR* dir, char* buf, YuDirEntry* out) {
    while (struct dirent* entry = readdir(dir)) {
        const char* name = entry->d_name;
#if defined(__APPLE__)
        size_t length = entry->d_namlen;
#else
        size_t length = strlen(name);
#endif
        if (name[0] == '.' && (length == 1 || (length == 2 && name[1] == '.'))) {
            continue;
        }

        uint32_t kind = entry->d_type;
        if (kind == DT_UNKNOWN) {
            struct stat info;
            if (fstatat(dirfd(dir), name, &info, AT_SYMLINK_NOFOLLOW) == 0) {
                kind = IFTODT(info.st_mode);
            }
        }

        memcpy(buf, name, length);
        out->inode = entry->d_ino;
        out->nameLength = static_cast<uint32_t>(length);
        out->kind = kind;
        return true;
    }
    return false;
}

// *************************************************************************************************

extern "C" int
yumiGetErrNo() {
    return errno;
}

extern "C" void
yumiPrint(const char* text) {
    const YumiStringHeader* header = (const YumiStringHeader*)(text - sizeof(YumiStringHeader));
    fwrite(text, 1, header->byteLength, stdout);
    fputc('\n', stdout);
    fflush(stdout);
}

extern "C" void
yumiTerminate() {
    std::terminate();
}

struct CrashMe {
    const char* name;
};

extern "C" void
yumiCrash() {
    struct CrashMe* x = NULL;
    x->name = "crash";
}

// **********************************************************************************************

extern "C" uint32_t
yumiObjectPointer(const char* object) {
    return (uintptr_t)object;
}

extern "C" uint32_t
yumiStringHash(const char* key, size_t length) {
    return hashlittle((const void*)key, length, 1);
}

extern "C" double
getInfinity() {
    return INFINITY;
}

// **********************************************************************************************

extern "C" int
yumiopen(const char* fd, int mode, int perms) {
    return open(fd, mode, perms);
}

extern "C" int
yumifcntl(int fd, int cmd, int arg) {
    return fcntl(fd, cmd, arg);
}

extern "C" int
yumiioctl(int fd, unsigned long request, struct winsize* size) {
    return ioctl(fd, request, size);
}

#if defined(__APPLE__) && TARGET_OS_OSX
static int yumiOpenDescriptors(int** outDescriptors) {
    *outDescriptors = NULL;
    int bytes = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, NULL, 0);
    if (bytes <= 0) {
        return 0;
    }
    int capacity = bytes / (int)sizeof(struct proc_fdinfo) + 32;
    struct proc_fdinfo* infos = (struct proc_fdinfo*)malloc(capacity * sizeof(struct proc_fdinfo));
    if (!infos) {
        return 0;
    }
    bytes = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, infos, capacity * (int)sizeof(struct proc_fdinfo));
    int count = bytes > 0 ? bytes / (int)sizeof(struct proc_fdinfo) : 0;
    int* descriptors = (int*)malloc((count > 0 ? count : 1) * sizeof(int));
    if (!descriptors) {
        free(infos);
        return 0;
    }
    for (int index = 0; index < count; index++) {
        descriptors[index] = infos[index].proc_fd;
    }
    free(infos);
    *outDescriptors = descriptors;
    return count;
}
#elif defined(__APPLE__)
static int yumiOpenDescriptors(int** outDescriptors) {
    *outDescriptors = NULL;
    int limit = getdtablesize();
    int* descriptors = (int*)malloc((limit > 0 ? limit : 1) * sizeof(int));
    if (!descriptors) {
        return 0;
    }
    int count = 0;
    for (int fd = 0; fd < limit; fd++) {
        if (fcntl(fd, F_GETFD) != -1) {
            descriptors[count++] = fd;
        }
    }
    *outDescriptors = descriptors;
    return count;
}
#else
static int yumiOpenDescriptors(int** outDescriptors) {
    *outDescriptors = NULL;
    DIR* listing = opendir("/proc/self/fd");
    if (!listing) {
        return 0;
    }
    int capacity = 64;
    int count = 0;
    int* descriptors = (int*)malloc(capacity * sizeof(int));
    while (descriptors) {
        struct dirent* entry = readdir(listing);
        if (!entry) {
            break;
        }
        char* end = NULL;
        long descriptor = strtol(entry->d_name, &end, 10);
        if (end == entry->d_name || *end) {
            continue;
        }
        if (count == capacity) {
            capacity *= 2;
            int* grown = (int*)realloc(descriptors, capacity * sizeof(int));
            if (!grown) {
                free(descriptors);
                descriptors = NULL;
                count = 0;
                break;
            }
            descriptors = grown;
        }
        descriptors[count++] = (int)descriptor;
    }
    closedir(listing);
    *outDescriptors = descriptors;
    return count;
}
#endif

extern "C" int
yumiSpawnPty(
    const char* path,
    char* const* argv,
    char* const* envp,
    const char* directory,
    int columns,
    int rows,
    int* outMaster) {
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) {
        return -errno;
    }
    char slaveName[128];
    if (grantpt(master) != 0 || unlockpt(master) != 0 || ptsname_r(master, slaveName, sizeof(slaveName)) != 0) {
        int error = errno;
        close(master);
        return -error;
    }
    fcntl(master, F_SETFD, FD_CLOEXEC);

    struct winsize size = {};
    size.ws_row = (unsigned short)rows;
    size.ws_col = (unsigned short)columns;

    int status[2];
    if (pipe(status) != 0) {
        int error = errno;
        close(master);
        return -error;
    }
    fcntl(status[0], F_SETFD, FD_CLOEXEC);
    fcntl(status[1], F_SETFD, FD_CLOEXEC);

    int* descriptors = NULL;
    int descriptorCount = yumiOpenDescriptors(&descriptors);
    bool hasDirectory = directory && directory[0];

    pid_t pid = fork();
    if (pid < 0) {
        int error = errno;
        free(descriptors);
        close(status[0]);
        close(status[1]);
        close(master);
        return -error;
    }

    if (pid == 0) {
        sigset_t empty;
        sigemptyset(&empty);
        sigprocmask(SIG_SETMASK, &empty, NULL);
        struct sigaction defaultAction = {};
        defaultAction.sa_handler = SIG_DFL;
        for (int signal = 1; signal < NSIG; signal++) {
            sigaction(signal, &defaultAction, NULL);
        }

        int error = 0;
        if (setsid() < 0) {
            error = errno;
        }
        int slave = error ? -1 : open(slaveName, O_RDWR);
        if (!error && slave < 0) {
            error = errno;
        }
        if (!error && ioctl(slave, TIOCSCTTY, 0) < 0) {
            error = errno;
        }
        if (!error) {
            ioctl(slave, TIOCSWINSZ, &size);
        }
        if (!error && (dup2(slave, 0) < 0 || dup2(slave, 1) < 0 || dup2(slave, 2) < 0)) {
            error = errno;
        }
        if (!error) {
            for (int index = 0; index < descriptorCount; index++) {
                int descriptor = descriptors[index];
                if (descriptor > 2 && descriptor != status[1]) {
                    close(descriptor);
                }
            }
            if (slave > 2) {
                close(slave);
            }
            if (hasDirectory && chdir(directory) < 0) {
                error = errno;
            }
        }
        if (!error) {
            execve(path, argv, envp);
            error = errno;
        }
        ssize_t ignored = write(status[1], &error, sizeof(error));
        (void)ignored;
        _exit(127);
    }

    free(descriptors);
    close(status[1]);
    int childError = 0;
    ssize_t received;
    do {
        received = read(status[0], &childError, sizeof(childError));
    } while (received < 0 && errno == EINTR);
    close(status[0]);
    if (received == (ssize_t)sizeof(childError)) {
        int reaped = 0;
        waitpid(pid, &reaped, 0);
        close(master);
        return -childError;
    }
    *outMaster = master;
    return pid;
}

// **********************************************************************************************

typedef struct {
    int x;
} Foo;

extern "C" void
dumpbuf(char* buf, size_t size) {
    Foo** buf2 = (Foo**)buf;
    printf("----- (%d %d)\n", size, sizeof(Foo*));
    for (size_t i = 0; i < size / 8; ++i) {
        printf("%d: %lld\n", i, buf2[i]);
    }
}

namespace {
struct ArrayViewOwner {
    char* data;
    size_t byteSize;
    void* arena;
    size_t incarnation;
    size_t generation;
    bool valid;
};
std::mutex arrayViewMutex;
std::vector<size_t> arrayViewOwners;
std::atomic<size_t> arrayViewOwnerCount {0};
}

extern "C" char*
yumiArrayViewCreate(char* data, size_t byteSize) {
    void* dataRoot = yumiAddTemporaryRoot(data);
    char* allocation = yumiAllocObject(sizeof(ArrayViewOwner));
    void* ownerRoot = yumiAddTemporaryRoot(allocation);
    ArrayViewOwner* owner = reinterpret_cast<ArrayViewOwner*>(allocation);
    owner->data = data;
    owner->byteSize = byteSize;
    owner->arena = yumiArenaFindOwner(data);
    owner->incarnation = owner->arena ? yumiArenaIncarnation(owner->arena) : 0;
    owner->generation = owner->arena ? yumiArenaGeneration(owner->arena, owner->incarnation) : 0;
    owner->valid = true;
    size_t handle = yumiGCRegisterOwnerWeak(allocation, allocation);
    {
        std::lock_guard<std::mutex> lock(arrayViewMutex);
        for (auto it = arrayViewOwners.begin(); it != arrayViewOwners.end();) {
            if (!yumiGCOwnerWeakLoad(*it)) {
                yumiGCReleaseOwnerWeak(*it);
                it = arrayViewOwners.erase(it);
            } else {
                ++it;
            }
        }
        arrayViewOwners.push_back(handle);
        arrayViewOwnerCount.store(arrayViewOwners.size(), std::memory_order_release);
    }
    yumiRemoveTemporaryRoot(ownerRoot);
    yumiRemoveTemporaryRoot(dataRoot);
    return allocation;
}

extern "C" YUMI_GC_LEAF bool
yumiArrayViewValid(char* allocation) {
    const ArrayViewOwner* owner = reinterpret_cast<const ArrayViewOwner*>(allocation);
    return owner && owner->valid && (!owner->arena ||
        (yumiArenaState(owner->arena, owner->incarnation) != 2 &&
         yumiArenaGeneration(owner->arena, owner->incarnation) == owner->generation));
}

extern "C" void
yumiArrayViewsInvalidate(char* data, size_t byteSize) {
    if (!byteSize || !arrayViewOwnerCount.load(std::memory_order_acquire)) {
        return;
    }
    const uintptr_t start = reinterpret_cast<uintptr_t>(data);
    std::lock_guard<std::mutex> lock(arrayViewMutex);
    for (auto it = arrayViewOwners.begin(); it != arrayViewOwners.end();) {
        ArrayViewOwner* owner = reinterpret_cast<ArrayViewOwner*>(yumiGCOwnerWeakLoad(*it));
        if (!owner) {
            yumiGCReleaseOwnerWeak(*it);
            it = arrayViewOwners.erase(it);
            continue;
        }
        const uintptr_t address = reinterpret_cast<uintptr_t>(owner->data);
        if (address >= start ? address - start < byteSize : start - address < owner->byteSize) {
            owner->valid = false;
        }
        ++it;
    }
    arrayViewOwnerCount.store(arrayViewOwners.size(), std::memory_order_release);
}

extern "C" void
yumiArrayViewsReplace(char* previous, size_t previousSize, char* next, size_t nextSize) {
    if (previous != next || nextSize < previousSize) {
        yumiArrayViewsInvalidate(previous, previousSize);
    }
}
