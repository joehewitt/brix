#ifndef BX_SYSTEM_H
#define BX_SYSTEM_H

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>

struct sockaddr;

struct BxAddressInfo {
    int32_t family;
    int32_t socktype;
    int32_t protocol;
    uint32_t addressLength;
    struct sockaddr* address;
};

extern "C" int brixGetAddressInfo(
    const char* host,
    const char* service,
    int flags,
    int family,
    int socktype,
    int protocol,
    BxAddressInfo* out);

// *************************************************************************************************

#ifdef __APPLE__
#define BX_O_EVTONLY O_EVTONLY
#else
#define BX_O_EVTONLY O_RDONLY
#endif

struct BxFileTime {
    int64_t seconds;
    int64_t nanoseconds;
};

struct BxFileStat {
    uint64_t device;
    uint64_t inode;
    uint32_t mode;
    uint32_t linkCount;
    uint32_t user;
    uint32_t group;
    int64_t size;
    int64_t blocks;
    int64_t blockSize;
    BxFileTime accessed;
    BxFileTime modified;
    BxFileTime changed;
    BxFileTime created;
};

extern "C" int brixStat(const char* path, BxFileStat* out);
extern "C" int brixLinkStat(const char* path, BxFileStat* out);

struct BxGlob {
    size_t count;
    char** paths;
    void* handle;
};

extern "C" int brixGlob(const char* pattern, BxGlob* out);
extern "C" void brixGlobFree(BxGlob* glob);

extern "C" int brixRenameExclusive(const char* from, const char* to);
extern "C" int brixCloneFile(const char* from, const char* to);
extern "C" int brixExecutablePath(char* buffer, uint32_t size);

// *************************************************************************************************

extern "C" char** brixEnvironment();

extern "C" int32_t brixSpawn(
    const char* name,
    char* const* argv,
    char* const* envp,
    const char* directory,
    const int32_t* streams,
    const int32_t* passed,
    int32_t passedCount,
    int32_t leadsGroup);

struct BxProcessUsage {
    int64_t exitCode;
    int64_t signal;
    int64_t userNanoseconds;
    int64_t systemNanoseconds;
    int64_t involuntarySwitches;
    uint64_t instructions;
    uint64_t cycles;
};

extern "C" int brixReapProcess(int32_t pid, BxProcessUsage* out);

extern "C" int32_t brixChildProcesses(int32_t parent, int32_t* pids, int32_t capacity);
extern "C" int32_t brixProcessGroupMembers(int32_t group, int32_t* pids, int32_t capacity);
extern "C" int32_t brixProcessDirectory(int32_t pid, char* buffer, uint32_t size);

extern "C" int brixSetSignalIgnored(int number, int ignored);

extern "C" int32_t brixWatchMemoryPressure();
extern "C" void brixUnwatchMemoryPressure(int32_t fd);
extern "C" int32_t brixTakeMemoryPressure(int32_t fd);
extern "C" void brixNoteMemoryPressure(int32_t level);

#endif
