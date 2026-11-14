#include <atomic>
#include <vector>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <YuEvents/YuSelectLoop.h>
#include <YuEvents/YuKQueueLoop.h>
#include <YuEvents/YuTaskQueue.h>
std::atomic<bool> interruptRead {false};
std::atomic<bool> interruptSend {false};
std::atomic<bool> interruptFlags {false};
int lastWriter = -1;
static std::function<void()> drainHook;
void finalDrainRace() { if (drainHook) drainHook(); }
YuTaskQueue* YuTask::getTaskQueue() { static YuTaskQueue queue; return &queue; }
extern "C" uint64_t yumiMonotonicTimeNanoseconds() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
static void require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
static int queued(YuTriggerFile& trigger) {
    int size = 0;
    require(ioctl(trigger.fd, FIONREAD, &size) == 0, "FIONREAD failed");
    return size;
}
static bool ready(YuTriggerFile& trigger) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(trigger.fd, &set);
    timeval timeout {0, 100000};
    return select(trigger.fd + 1, &set, nullptr, nullptr, &timeout) == 1;
}
static int descriptorCount() {
    int count = 0;
    for (int fd = 0; fd < 1024; ++fd) if (fcntl(fd, F_GETFD) != -1) ++count;
    return count;
}
template<class Loop> static void restart() {
    Loop loop;
    for (int i = 0; i < 8; ++i) {
        loop.start();
        loop.stop();
        loop.join();
    }
}
static void boundaries() {
    for (size_t size : {0, 1, 1023, 1024, 1025, 2048, 8192}) {
        YuTriggerFile trigger;
        int writer = lastWriter;
        std::vector<char> bytes(size, 'x');
        if (size) require(send(writer, bytes.data(), size, 0) == static_cast<ssize_t>(size), "boundary setup failed");
        trigger.flush();
        require(queued(trigger) == 0, "finite boundary not drained");
        trigger.flush();
    }
    YuTriggerFile trigger;
    int writer = lastWriter;
    char bytes[1024] {};
    while (send(writer, bytes, sizeof(bytes), 0) > 0) {}
    require(errno == EAGAIN || errno == EWOULDBLOCK, "saturation setup failed");
    require(queued(trigger) > 0, "saturation lost readiness");
    trigger.trigger();
    do { trigger.flush(); } while (queued(trigger));
    interruptRead = true;
    trigger.trigger();
    trigger.flush();
    require(queued(trigger) == 0, "interrupted drain left pending wakeups");
    interruptSend = true;
    trigger.trigger();
    require(ready(trigger), "interrupted publication lost wakeup");
    trigger.flush();
    interruptFlags = true;
    { YuTriggerFile interruptedFlags; }
}
static void concurrency() {
    YuTriggerFile trigger;
    std::mutex lock;
    size_t pending = 0;
    std::vector<std::thread> producers;
    for (int producer = 0; producer < 4; ++producer) {
        producers.emplace_back([&]() {
            for (int i = 0; i < 5000; ++i) {
                { std::lock_guard<std::mutex> guard(lock); ++pending; }
                trigger.trigger();
            }
        });
    }
    size_t applied = 0;
    while (applied != 20000) {
        require(ready(trigger), "finite producer work stranded");
        trigger.flush();
        std::lock_guard<std::mutex> guard(lock);
        applied += pending;
        pending = 0;
    }
    for (auto& producer : producers) producer.join();
    trigger.flush();
    require(queued(trigger) == 0, "finite producers did not quiesce");
    std::mutex raceLock;
    std::condition_variable condition;
    bool draining = false;
    bool published = false;
    std::thread producer([&]() {
        std::unique_lock<std::mutex> guard(raceLock);
        condition.wait(guard, [&]() { return draining; });
        trigger.trigger();
        published = true;
        condition.notify_one();
    });
    drainHook = [&]() {
        std::unique_lock<std::mutex> guard(raceLock);
        draining = true;
        condition.notify_one();
        condition.wait(guard, [&]() { return published; });
    };
    trigger.flush();
    producer.join();
    drainHook = nullptr;
    require(ready(trigger), "publication racing final drain lost wakeup");
    trigger.flush();
}
static void selectLoops() {
    for (int cycle = 0; cycle < 8; ++cycle) {
        int sockets[2];
        require(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "loop sockets failed");
        YuSelectLoop loop;
        YuTask tasks[512];
        YuTask observer;
        auto canceledWait = [&](YuTask* task) {
            YuSelectWaiter waiter(task, sockets[0], true, 0);
            loop.beginWait(&waiter);
            loop.cancelDescriptor(sockets[0]);
            loop.endWait(&waiter);
        };
        for (int i = 0; i < 512; ++i) canceledWait(&tasks[i]);
        YuSelectWaiter observed(&observer, sockets[0], true, 0);
        loop.beginWait(&observed);
        require(send(sockets[1], "x", 1, 0) == 1, "observer setup failed");
        loop.start();
        while (!observer.completed.load()) std::this_thread::yield();
        loop.endWait(&observed);
        std::atomic<bool> publish {true};
        std::atomic<bool> started {false};
        std::thread producer([&]() {
            while (publish.load()) {
                canceledWait(&tasks[0]);
                started = true;
            }
        });
        while (!started.load()) std::this_thread::yield();
        loop.stop();
        publish = false;
        producer.join();
        loop.join();
        ::close(sockets[0]);
        ::close(sockets[1]);
    }
}
static void kqueueLoops() {
    for (int cycle = 0; cycle < 8; ++cycle) {
        int sockets[2];
        require(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "loop sockets failed");
        YuKQueueLoop loop;
        YuTask tasks[512];
        YuTask observer;
        uint64_t deadline = yumiMonotonicTimeNanoseconds() + 3600000000000ull;
        auto canceledWait = [&](YuTask* task) {
            YuPollWaiter waiter(task, sockets[0], EVFILT_READ);
            loop.beginWait(&waiter, deadline);
            loop.releaseDescriptor(sockets[0]);
            loop.endWait(&waiter);
        };
        loop.start();
        for (int i = 0; i < 512; ++i) canceledWait(&tasks[i]);
        YuPollWaiter observed(&observer, sockets[0], EVFILT_READ);
        require(loop.beginWait(&observed, deadline) == YuPollWaitStartSuspend, "observer did not wait");
        require(send(sockets[1], "x", 1, 0) == 1, "observer setup failed");
        while (!observer.completed.load()) std::this_thread::yield();
        loop.endWait(&observed);
        std::atomic<bool> publish {true};
        std::atomic<bool> started {false};
        std::thread producer([&]() {
            while (publish.load()) {
                canceledWait(&tasks[0]);
                started = true;
            }
        });
        while (!started.load()) std::this_thread::yield();
        loop.stop();
        publish = false;
        producer.join();
        loop.join();
        ::close(sockets[0]);
        ::close(sockets[1]);
    }
}
int main() {
    alarm(15);
    int before = descriptorCount();
    boundaries();
    concurrency();
    selectLoops();
    kqueueLoops();
    restart<YuSelectLoop>();
    restart<YuKQueueLoop>();
    require(descriptorCount() == before, "trigger/loop lifecycle leaked descriptors");
    int savedInput = dup(0);
    ::close(0);
    int replacement;
    {
        YuTriggerFile trigger;
        trigger.close();
        replacement = open("/dev/null", O_RDONLY);
        require(replacement == 0, "zero descriptor not released");
        trigger.close();
    }
    require(fcntl(replacement, F_GETFD) != -1, "repeated close closed a reused descriptor");
    if (savedInput != -1) { dup2(savedInput, 0); ::close(savedInput); }
    else ::close(replacement);
    return 0;
}
