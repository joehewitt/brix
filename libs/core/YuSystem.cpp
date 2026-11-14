#include "YuCore/YuSystem.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <netdb.h>
#include <signal.h>
#include <spawn.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <crt_externs.h>
#include <mach-o/dyld.h>
#include <sys/clonefile.h>
#if TARGET_OS_OSX
#include <libproc.h>
#endif
#else
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#endif

extern "C" int
yumiGetAddressInfo(
    const char* host,
    const char* service,
    int flags,
    int family,
    int socktype,
    int protocol,
    YuAddressInfo* out) {
    struct addrinfo hints = {};
    hints.ai_flags = flags;
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    struct addrinfo* found = NULL;
    int error = getaddrinfo(host, service, &hints, &found);
    if (error != 0) {
        return error;
    }
    struct sockaddr* address = (struct sockaddr*)malloc(found->ai_addrlen);
    if (!address) {
        freeaddrinfo(found);
        return EAI_MEMORY;
    }
    memcpy(address, found->ai_addr, found->ai_addrlen);
    out->family = found->ai_family;
    out->socktype = found->ai_socktype;
    out->protocol = found->ai_protocol;
    out->addressLength = (uint32_t)found->ai_addrlen;
    out->address = address;
    freeaddrinfo(found);
    return 0;
}

// *************************************************************************************************

#if defined(__APPLE__)

static YuFileTime
fileTime(struct timespec time) {
    return {(int64_t)time.tv_sec, (int64_t)time.tv_nsec};
}

static int
fileStat(const char* path, bool followsLinks, YuFileStat* out) {
    struct stat info;
    if ((followsLinks ? stat(path, &info) : lstat(path, &info)) != 0) {
        return -1;
    }
    out->device = (uint64_t)(uint32_t)info.st_dev;
    out->inode = info.st_ino;
    out->mode = info.st_mode;
    out->linkCount = info.st_nlink;
    out->user = info.st_uid;
    out->group = info.st_gid;
    out->size = info.st_size;
    out->blocks = info.st_blocks;
    out->blockSize = info.st_blksize;
    out->accessed = fileTime(info.st_atimespec);
    out->modified = fileTime(info.st_mtimespec);
    out->changed = fileTime(info.st_ctimespec);
    out->created = fileTime(info.st_birthtimespec);
    return 0;
}

#else

static YuFileTime
fileTime(struct statx_timestamp time) {
    return {(int64_t)time.tv_sec, (int64_t)time.tv_nsec};
}

static int
fileStat(const char* path, bool followsLinks, YuFileStat* out) {
    struct statx info;
    int flags = followsLinks ? 0 : AT_SYMLINK_NOFOLLOW;
    if (statx(AT_FDCWD, path, flags, STATX_BASIC_STATS | STATX_BTIME, &info) != 0) {
        return -1;
    }
    out->device = makedev(info.stx_dev_major, info.stx_dev_minor);
    out->inode = info.stx_ino;
    out->mode = info.stx_mode;
    out->linkCount = info.stx_nlink;
    out->user = info.stx_uid;
    out->group = info.stx_gid;
    out->size = (int64_t)info.stx_size;
    out->blocks = (int64_t)info.stx_blocks;
    out->blockSize = info.stx_blksize;
    out->accessed = fileTime(info.stx_atime);
    out->modified = fileTime(info.stx_mtime);
    out->changed = fileTime(info.stx_ctime);
    out->created = (info.stx_mask & STATX_BTIME) ? fileTime(info.stx_btime) : YuFileTime{0, 0};
    return 0;
}

#endif

extern "C" int
yumiStat(const char* path, YuFileStat* out) {
    return fileStat(path, true, out);
}

extern "C" int
yumiLinkStat(const char* path, YuFileStat* out) {
    return fileStat(path, false, out);
}

extern "C" int
yumiGlob(const char* pattern, YuGlob* out) {
    *out = {};
    glob_t* found = (glob_t*)calloc(1, sizeof(glob_t));
    if (!found) {
        return GLOB_NOSPACE;
    }
    int error = glob(pattern, GLOB_TILDE, NULL, found);
    if (error != 0) {
        globfree(found);
        free(found);
        return error;
    }
    out->count = found->gl_pathc;
    out->paths = found->gl_pathv;
    out->handle = found;
    return 0;
}

extern "C" void
yumiGlobFree(YuGlob* glob) {
    glob_t* found = (glob_t*)glob->handle;
    if (found) {
        globfree(found);
        free(found);
    }
    *glob = {};
}

extern "C" int
yumiRenameExclusive(const char* from, const char* to) {
#if defined(__APPLE__)
    return renamex_np(from, to, RENAME_EXCL);
#else
    return renameat2(AT_FDCWD, from, AT_FDCWD, to, RENAME_NOREPLACE);
#endif
}

extern "C" int
yumiCloneFile(const char* from, const char* to) {
#if defined(__APPLE__)
    return clonefile(from, to, 0);
#else
    int source = open(from, O_RDONLY | O_CLOEXEC);
    if (source < 0) {
        return -1;
    }
    struct stat info;
    int target = fstat(source, &info) == 0
        ? open(to, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, info.st_mode & 07777)
        : -1;
    if (target < 0) {
        int error = errno;
        close(source);
        errno = error;
        return -1;
    }
    int status = ioctl(target, FICLONE, source);
    int error = errno;
    close(source);
    close(target);
    if (status != 0) {
        unlink(to);
        errno = error;
        return -1;
    }
    return 0;
#endif
}

extern "C" int
yumiExecutablePath(char* buffer, uint32_t size) {
#if defined(__APPLE__)
    return _NSGetExecutablePath(buffer, &size);
#else
    ssize_t length = readlink("/proc/self/exe", buffer, size);
    if (length < 0 || (size_t)length >= size) {
        return -1;
    }
    buffer[length] = 0;
    return 0;
#endif
}

// *************************************************************************************************

extern "C" char**
yumiEnvironment() {
#if defined(__APPLE__)
    return *_NSGetEnviron();
#else
    return environ;
#endif
}

static void
addStream(posix_spawn_file_actions_t* actions, int32_t source, int target, int flags) {
    if (source >= 0) {
        posix_spawn_file_actions_adddup2(actions, source, target);
    } else if (source == -2) {
        posix_spawn_file_actions_addopen(actions, target, "/dev/null", flags, 0);
    } else {
#if defined(__APPLE__)
        posix_spawn_file_actions_addinherit_np(actions, target);
#endif
    }
}

static int
addDirectory(posix_spawn_file_actions_t* actions, const char* directory) {
#if defined(__APPLE__) && !TARGET_OS_OSX
    (void)actions;
    (void)directory;
    return ENOTSUP;
#else
    return posix_spawn_file_actions_addchdir_np(actions, directory);
#endif
}

extern "C" int32_t
yumiSpawn(
    const char* name,
    char* const* argv,
    char* const* envp,
    const char* directory,
    const int32_t* streams,
    const int32_t* passed,
    int32_t passedCount,
    int32_t leadsGroup) {
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    int error = directory && directory[0] ? addDirectory(&actions, directory) : 0;
    if (error != 0) {
        posix_spawn_file_actions_destroy(&actions);
        return -error;
    }
    addStream(&actions, streams[0], 0, O_RDONLY);
    addStream(&actions, streams[1], 1, O_WRONLY);
    addStream(&actions, streams[2], 2, O_WRONLY);
    for (int32_t index = 0; index < passedCount; index++) {
        posix_spawn_file_actions_adddup2(&actions, passed[index], 3 + index);
    }
#if !defined(__APPLE__)
    posix_spawn_file_actions_addclosefrom_np(&actions, 3 + passedCount);
#endif

    posix_spawnattr_t attributes;
    posix_spawnattr_init(&attributes);
    short flags = POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK;
#if defined(__APPLE__)
    flags |= POSIX_SPAWN_CLOEXEC_DEFAULT;
#endif
    if (leadsGroup) {
        flags |= POSIX_SPAWN_SETPGROUP;
        posix_spawnattr_setpgroup(&attributes, 0);
    }
    posix_spawnattr_setflags(&attributes, flags);
    sigset_t defaults;
    sigfillset(&defaults);
    posix_spawnattr_setsigdefault(&attributes, &defaults);
    sigset_t mask;
    sigemptyset(&mask);
    posix_spawnattr_setsigmask(&attributes, &mask);

    pid_t child = 0;
    error = posix_spawnp(&child, name, &actions, &attributes, argv, envp);
    posix_spawnattr_destroy(&attributes);
    posix_spawn_file_actions_destroy(&actions);
    return error != 0 ? -error : child;
}

#if defined(__APPLE__) && TARGET_OS_OSX

static int32_t
listProcesses(uint32_t kind, int32_t value, int32_t* pids, int32_t capacity) {
    int filled = proc_listpids(kind, (uint32_t)value, pids, capacity * (int)sizeof(int32_t));
    return filled <= 0 ? filled : filled / (int32_t)sizeof(int32_t);
}

extern "C" int32_t
yumiChildProcesses(int32_t parent, int32_t* pids, int32_t capacity) {
    return listProcesses(PROC_PPID_ONLY, parent, pids, capacity);
}

extern "C" int32_t
yumiProcessGroupMembers(int32_t group, int32_t* pids, int32_t capacity) {
    return listProcesses(PROC_PGRP_ONLY, group, pids, capacity);
}

extern "C" int32_t
yumiProcessDirectory(int32_t pid, char* buffer, uint32_t size) {
    struct proc_vnodepathinfo info;
    if (proc_pidinfo(pid, PROC_PIDVNODEPATHINFO, 0, &info, sizeof(info)) != (int)sizeof(info)) {
        return -1;
    }
    size_t length = strlen(info.pvi_cdir.vip_path);
    if (length == 0 || length >= size) {
        return -1;
    }
    memcpy(buffer, info.pvi_cdir.vip_path, length + 1);
    return (int32_t)length;
}

#elif defined(__APPLE__)

extern "C" int32_t
yumiChildProcesses(int32_t, int32_t*, int32_t) {
    errno = ENOTSUP;
    return -1;
}

extern "C" int32_t
yumiProcessGroupMembers(int32_t, int32_t*, int32_t) {
    errno = ENOTSUP;
    return -1;
}

extern "C" int32_t
yumiProcessDirectory(int32_t, char*, uint32_t) {
    errno = ENOTSUP;
    return -1;
}

#else

static int32_t
listProcesses(bool byGroup, int32_t value, int32_t* pids, int32_t capacity) {
    DIR* processes = opendir("/proc");
    if (!processes) {
        return -1;
    }
    int32_t count = 0;
    while (struct dirent* entry = readdir(processes)) {
        char* end = NULL;
        long pid = strtol(entry->d_name, &end, 10);
        if (pid <= 0 || *end) {
            continue;
        }
        char path[64];
        snprintf(path, sizeof(path), "/proc/%ld/stat", pid);
        int file = open(path, O_RDONLY | O_CLOEXEC);
        if (file < 0) {
            continue;
        }
        char line[512];
        ssize_t length = read(file, line, sizeof(line) - 1);
        close(file);
        if (length <= 0) {
            continue;
        }
        line[length] = 0;
        const char* name = strrchr(line, ')');
        char state = 0;
        int parent = 0;
        int group = 0;
        if (!name || sscanf(name + 1, " %c %d %d", &state, &parent, &group) != 3) {
            continue;
        }
        if ((byGroup ? group : parent) != value) {
            continue;
        }
        if (count == capacity) {
            break;
        }
        pids[count++] = (int32_t)pid;
    }
    closedir(processes);
    return count;
}

extern "C" int32_t
yumiChildProcesses(int32_t parent, int32_t* pids, int32_t capacity) {
    return listProcesses(false, parent, pids, capacity);
}

extern "C" int32_t
yumiProcessGroupMembers(int32_t group, int32_t* pids, int32_t capacity) {
    return listProcesses(true, group, pids, capacity);
}

extern "C" int32_t
yumiProcessDirectory(int32_t pid, char* buffer, uint32_t size) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/cwd", pid);
    ssize_t length = readlink(path, buffer, size);
    if (length <= 0 || (size_t)length >= size) {
        return -1;
    }
    buffer[length] = 0;
    return (int32_t)length;
}

#endif

static int64_t
timeNanoseconds(const struct timeval& time) {
    return (int64_t)time.tv_sec * 1000000000 + (int64_t)time.tv_usec * 1000;
}

extern "C" int
yumiReapProcess(int32_t pid, YuProcessUsage* out) {
    memset(out, 0, sizeof(*out));
#if defined(__APPLE__) && TARGET_OS_OSX
    struct rusage_info_v4 counters;
    if (proc_pid_rusage(pid, RUSAGE_INFO_V4, (rusage_info_t*)&counters) == 0) {
        out->instructions = counters.ri_instructions;
        out->cycles = counters.ri_cycles;
    }
#endif
    int status = 0;
    struct rusage usage;
    pid_t reaped;
    do {
        reaped = wait4(pid, &status, 0, &usage);
    } while (reaped < 0 && errno == EINTR);
    if (reaped < 0) {
        return errno;
    }
    if (WIFSIGNALED(status)) {
        out->signal = WTERMSIG(status);
    } else {
        out->exitCode = WEXITSTATUS(status);
    }
    out->userNanoseconds = timeNanoseconds(usage.ru_utime);
    out->systemNanoseconds = timeNanoseconds(usage.ru_stime);
    out->involuntarySwitches = usage.ru_nivcsw;
    return 0;
}

extern "C" int
yumiSetSignalIgnored(int number, int ignored) {
    struct sigaction action = {};
    action.sa_handler = ignored ? SIG_IGN : SIG_DFL;
    sigemptyset(&action.sa_mask);
    return sigaction(number, &action, NULL);
}
