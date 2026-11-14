#include <YuEvents/YuEvents.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

extern "C" int
yumiSharedMemoryCreate(const char* name, int64_t size) {
    if (!name || size <= 0) {
        return -1;
    }
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        return -1;
    }
    if (ftruncate(fd, static_cast<off_t>(size)) != 0) {
        close(fd);
        shm_unlink(name);
        return -1;
    }
    return fd;
}

extern "C" int
yumiSharedMemoryOpen(const char* name) {
    if (!name) {
        return -1;
    }
    return shm_open(name, O_RDONLY, 0);
}

extern "C" bool
yumiSharedMemoryUnlink(const char* name) {
    return name && shm_unlink(name) == 0;
}

extern "C" void*
yumiSharedMemoryMap(int fd, int64_t size, bool writable) {
    if (fd < 0 || size <= 0) {
        return nullptr;
    }
    int protection = writable ? PROT_READ | PROT_WRITE : PROT_READ;
    void* memory = mmap(nullptr, static_cast<size_t>(size), protection, MAP_SHARED, fd, 0);
    return memory == MAP_FAILED ? nullptr : memory;
}

extern "C" void
yumiSharedMemoryUnmap(void* memory, int64_t size) {
    if (memory && size > 0) {
        munmap(memory, static_cast<size_t>(size));
    }
}

extern "C" bool
yumiSocketPair(int* fds) {
    if (!fds) {
        return false;
    }
    int result;
    do {
        result = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        return false;
    }
    for (int index = 0; index < 2; ++index) {
#if defined(SO_NOSIGPIPE)
        int enabled = 1;
        setsockopt(fds[index], SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
#endif
        int flags = fcntl(fds[index], F_GETFD);
        if (flags >= 0) {
            fcntl(fds[index], F_SETFD, flags | FD_CLOEXEC);
        }
    }
    return true;
}
