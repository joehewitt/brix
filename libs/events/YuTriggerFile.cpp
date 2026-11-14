
#include <YuEvents/YuTriggerFile.h>

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <fcntl.h>
#include <cstdio>
#include <cstring>

// *************************************************************************************************

static void
failTrigger(const char* operation) {
    std::fprintf(stderr, "Event-loop wakeup %s failed: %s\n", operation, std::strerror(errno));
    std::abort();
}

YuTriggerFile::YuTriggerFile() : fd(-1), fdOut(-1) {
    int fds[2];
    int result;
    do {
        result = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        failTrigger("socketpair");
    }
    fdOut = fds[0];
    fd = fds[1];
    for (int endpoint : fds) {
        int flags;
        do {
            flags = fcntl(endpoint, F_GETFL);
        } while (flags == -1 && errno == EINTR);
        if (flags == -1) failTrigger("get flags");
        do {
            result = fcntl(endpoint, F_SETFL, flags | O_NONBLOCK);
        } while (result == -1 && errno == EINTR);
        if (result == -1) failTrigger("set nonblocking");
    }
#ifdef SO_NOSIGPIPE
    int enabled = 1;
    do {
        result = setsockopt(fdOut, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        failTrigger("disable SIGPIPE");
    }
#endif
}

YuTriggerFile::~YuTriggerFile() {
    close();
}

void
YuTriggerFile::trigger() {
    ssize_t ret;
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    do {
        ret = send(fdOut, "x", 1, flags);
    } while (ret == -1 && errno == EINTR);
    if (ret == 1 || (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
        return;
    }
    if (ret != -1) errno = EIO;
    failTrigger("send");
}

void
YuTriggerFile::flush() {
    char buf[1024];
    constexpr size_t maxReadAttempts = 64;
    for (size_t attempt = 0; attempt < maxReadAttempts; ++attempt) {
        ssize_t ret = read(fd, buf, sizeof(buf));
        if (ret > 0) {
            continue;
        } else if (ret == 0) {
            errno = EPIPE;
            failTrigger("receive EOF");
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        } else if (errno == EINTR) {
            continue;
        } else {
            failTrigger("receive");
        }
    }
}

void
YuTriggerFile::close() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
    if (fdOut >= 0) {
        ::close(fdOut);
        fdOut = -1;
    }
}
