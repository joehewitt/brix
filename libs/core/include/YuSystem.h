#ifndef YU_SYSTEM_H
#define YU_SYSTEM_H

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>

struct sockaddr;

struct YuAddressInfo {
    int32_t family;
    int32_t socktype;
    int32_t protocol;
    uint32_t addressLength;
    struct sockaddr* address;
};

extern "C" int yumiGetAddressInfo(
    const char* host,
    const char* service,
    int flags,
    int family,
    int socktype,
    int protocol,
    YuAddressInfo* out);

// *************************************************************************************************

#ifdef __APPLE__
#define YU_O_EVTONLY O_EVTONLY
#else
#define YU_O_EVTONLY O_RDONLY
#endif

struct YuFileTime {
    int64_t seconds;
    int64_t nanoseconds;
};

struct YuFileStat {
    uint64_t device;
    uint64_t inode;
    uint32_t mode;
    uint32_t linkCount;
    uint32_t user;
    uint32_t group;
    int64_t size;
    int64_t blocks;
    int64_t blockSize;
    YuFileTime accessed;
    YuFileTime modified;
    YuFileTime changed;
    YuFileTime created;
};

extern "C" int yumiStat(const char* path, YuFileStat* out);
extern "C" int yumiLinkStat(const char* path, YuFileStat* out);

struct YuGlob {
    size_t count;
    char** paths;
    void* handle;
};

extern "C" int yumiGlob(const char* pattern, YuGlob* out);
extern "C" void yumiGlobFree(YuGlob* glob);

extern "C" int yumiRenameExclusive(const char* from, const char* to);
extern "C" int yumiCloneFile(const char* from, const char* to);
extern "C" int yumiExecutablePath(char* buffer, uint32_t size);

// *************************************************************************************************

extern "C" char** yumiEnvironment();

extern "C" int32_t yumiSpawn(
    const char* name,
    char* const* argv,
    char* const* envp,
    const char* directory,
    const int32_t* streams,
    const int32_t* passed,
    int32_t passedCount,
    int32_t leadsGroup);

struct YuProcessUsage {
    int64_t exitCode;
    int64_t signal;
    int64_t userNanoseconds;
    int64_t systemNanoseconds;
    int64_t involuntarySwitches;
    uint64_t instructions;
    uint64_t cycles;
};

extern "C" int yumiReapProcess(int32_t pid, YuProcessUsage* out);

extern "C" int32_t yumiChildProcesses(int32_t parent, int32_t* pids, int32_t capacity);
extern "C" int32_t yumiProcessGroupMembers(int32_t group, int32_t* pids, int32_t capacity);
extern "C" int32_t yumiProcessDirectory(int32_t pid, char* buffer, uint32_t size);

extern "C" int yumiSetSignalIgnored(int number, int ignored);

extern "C" int32_t yumiWatchMemoryPressure();
extern "C" void yumiUnwatchMemoryPressure(int32_t fd);
extern "C" int32_t yumiTakeMemoryPressure(int32_t fd);
extern "C" void yumiNoteMemoryPressure(int32_t level);

#endif
