#include "BxCore/BxProbe.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// *************************************************************************************************

static const uint32_t BRIX_PROBE_CONTROL_MAGIC = 0x49435059;
static const uint16_t BRIX_PROBE_CONTROL_VERSION = 1;
static const size_t BRIX_PROBE_CONTROL_HEADER_SIZE = 16;
static const size_t BRIX_PROBE_CONTROL_MAX_PAYLOAD = 64 * 1024;

static int gProbeControlFd = -1;
static std::vector<uint8_t> gProbeControlReadBuffer;
static pthread_mutex_t gProbeControlLock = PTHREAD_MUTEX_INITIALIZER;

class BxProbeControlLock {
public:
    BxProbeControlLock() { pthread_mutex_lock(&gProbeControlLock); }
    ~BxProbeControlLock() { pthread_mutex_unlock(&gProbeControlLock); }
};

static uint16_t
brixProbeControlReadU16(const uint8_t* data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t
brixProbeControlReadU32(const uint8_t* data) {
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static bool
brixProbeControlSetNonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

static void
brixProbeControlDrop() {
    if (gProbeControlFd >= 0) {
        close(gProbeControlFd);
        gProbeControlFd = -1;
    }
    gProbeControlReadBuffer.clear();
}

extern "C" bool
brixProbeControlOpen(const char* path, const char* /* token */) {
    BxProbeControlLock lock;
    brixProbeControlDrop();
    if (!path || !path[0]) {
        return false;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(path) >= sizeof(addr.sun_path)) {
        close(fd);
        return false;
    }
    strcpy(addr.sun_path, path);

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    addr.sun_len = SUN_LEN(&addr);
    socklen_t addrLength = addr.sun_len;
#else
    socklen_t addrLength = (socklen_t)(
        offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path) + 1);
#endif

    bool connected = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (connect(fd, (struct sockaddr*)&addr, addrLength) == 0) {
            connected = true;
            break;
        }
        if (errno != ENOENT && errno != ECONNREFUSED) {
            break;
        }
        usleep(10000);
    }

    if (!connected || !brixProbeControlSetNonblocking(fd)) {
        close(fd);
        return false;
    }

    gProbeControlFd = fd;
    return true;
}

extern "C" void
brixProbeControlClose() {
    BxProbeControlLock lock;
    brixProbeControlDrop();
}

static uint8_t gProbeLiveLock = 0;

extern "C" bool
brixProbeLiveTryLock() {
    return !__atomic_test_and_set(&gProbeLiveLock, __ATOMIC_ACQUIRE);
}

extern "C" void
brixProbeLiveLock() {
    while (__atomic_test_and_set(&gProbeLiveLock, __ATOMIC_ACQUIRE)) {
        sched_yield();
    }
}

extern "C" void
brixProbeLiveUnlock() {
    __atomic_clear(&gProbeLiveLock, __ATOMIC_RELEASE);
}

extern "C" bool
brixProbeControlAvailable() {
    BxProbeControlLock lock;
    return gProbeControlFd >= 0;
}

extern "C" size_t
brixProbeControlRead(uint8_t* dst, size_t capacity) {
    BxProbeControlLock lock;
    if (gProbeControlFd < 0 || !dst || capacity < BRIX_PROBE_CONTROL_HEADER_SIZE) {
        return 0;
    }

    uint8_t chunk[4096];
    while (true) {
        ssize_t amount = recv(gProbeControlFd, chunk, sizeof(chunk), 0);
        if (amount > 0) {
            gProbeControlReadBuffer.insert(
                gProbeControlReadBuffer.end(),
                chunk,
                chunk + (size_t)amount);
            continue;
        }
        if (amount == 0) {
            brixProbeControlDrop();
            return 0;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        brixProbeControlDrop();
        return 0;
    }

    if (gProbeControlReadBuffer.size() < BRIX_PROBE_CONTROL_HEADER_SIZE) {
        return 0;
    }

    const uint8_t* data = gProbeControlReadBuffer.data();
    uint32_t magic = brixProbeControlReadU32(data);
    uint16_t version = brixProbeControlReadU16(data + 4);
    uint32_t payloadLength = brixProbeControlReadU32(data + 12);
    if (
        magic != BRIX_PROBE_CONTROL_MAGIC ||
        version != BRIX_PROBE_CONTROL_VERSION ||
        payloadLength > BRIX_PROBE_CONTROL_MAX_PAYLOAD) {
        brixProbeControlDrop();
        return 0;
    }

    size_t frameLength = BRIX_PROBE_CONTROL_HEADER_SIZE + (size_t)payloadLength;
    if (gProbeControlReadBuffer.size() < frameLength || frameLength > capacity) {
        return 0;
    }

    memcpy(dst, gProbeControlReadBuffer.data(), frameLength);
    gProbeControlReadBuffer.erase(
        gProbeControlReadBuffer.begin(),
        gProbeControlReadBuffer.begin() + frameLength);
    return frameLength;
}

extern "C" bool
brixProbeControlWrite(const uint8_t* data, size_t size) {
    BxProbeControlLock lock;
    if (gProbeControlFd < 0 || !data || !size) {
        return false;
    }

    size_t written = 0;
    while (written < size) {
        ssize_t amount = send(gProbeControlFd, data + written, size - written, 0);
        if (amount > 0) {
            written += (size_t)amount;
            continue;
        }
        if (amount < 0 && errno == EINTR) {
            continue;
        }
        if (amount < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            usleep(1000);
            continue;
        }
        brixProbeControlDrop();
        return false;
    }
    return true;
}
