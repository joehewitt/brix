#include <sys/socket.h>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
extern std::atomic<bool> interruptRead;
extern std::atomic<bool> interruptSend;
extern std::atomic<bool> interruptFlags;
extern int lastWriter;
extern void finalDrainRace();
ssize_t wakeupRead(int fd, void* data, size_t size) {
    if (interruptRead.exchange(false)) { errno = EINTR; return -1; }
    ssize_t result = ::read(fd, data, size);
    if (result == -1 && errno == EAGAIN) finalDrainRace();
    return result;
}
ssize_t wakeupSend(int fd, const void* data, size_t size, int flags) {
    if (interruptSend.exchange(false)) { errno = EINTR; return -1; }
    return ::send(fd, data, size, flags);
}
int wakeupSocketpair(int domain, int type, int protocol, int* fds) {
    int result = ::socketpair(domain, type, protocol, fds);
    if (result == 0) lastWriter = fds[0];
    return result;
}
int wakeupFcntl(int fd, int operation, int value = 0) {
    if (interruptFlags.exchange(false)) { errno = EINTR; return -1; }
    return ::fcntl(fd, operation, value);
}
#define read wakeupRead
#define send wakeupSend
#define socketpair wakeupSocketpair
#define fcntl wakeupFcntl
#include "../../YuTriggerFile.cpp"
