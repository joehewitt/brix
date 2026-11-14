#include <YuEvents/YuEpollLoop.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuTaskQueue.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <unistd.h>

// *************************************************************************************************

static const uint64_t kYuDescriptorEvent = 0;
static const uint64_t kYuWakeEvent = 1;
static const uint64_t kYuTimerEvent = 2;
static const uint64_t kYuSignalEvent = 3;
static const uint64_t kYuProcessEvent = 4;
static const int kYuEventBatchSize = 128;

static std::atomic<int> gSignalWriteDescriptor(-1);

static void*
YuEpollLoopThread(void* input) {
    YuEpollLoop* loop = (YuEpollLoop*)input;
    loop->runEventLoop();

    pthread_exit(NULL);
}

static void
yumiScheduleTasks(std::vector<YuTask*>& tasks) {
    for (YuTask* task : tasks) {
        task->getTaskQueue()->scheduleTask(task);
    }
    tasks.clear();
}

static void
failLoop(const char* operation) {
    perror(operation);
    abort();
}

static uint64_t
eventTag(uint64_t kind, int value) {
    return (kind << 32) | static_cast<uint32_t>(value);
}

static void
drainDescriptor(int fd) {
    uint64_t buffer[8];
    while (read(fd, buffer, sizeof(buffer)) > 0) {
    }
}

static void
yumiSignalHandler(int sig) {
    int error = errno;
    unsigned char value = static_cast<unsigned char>(sig);
    ssize_t written = write(gSignalWriteDescriptor.load(std::memory_order_relaxed), &value, 1);
    (void)written;
    errno = error;
}

// *************************************************************************************************

YuEpollLoop::YuEpollLoop():
    epoll(-1),
    wakeDescriptor(-1),
    timerDescriptor(-1),
    signalDescriptor(-1),
    stopRequested(false),
    armedDeadlineNanoseconds(UINT64_MAX),
    timerDeadlineNanoseconds(UINT64_MAX)
{
    pthread_mutex_init(&lock, 0);
}

YuEpollLoop::~YuEpollLoop() {
    for (YuEpollDescriptor* descriptor : descriptors) {
        delete descriptor;
    }
    for (auto& entry : sleepTimers) {
        delete entry.second;
    }
    for (auto& entry : processWatches) {
        ::close(entry.second.descriptor);
    }
    pthread_mutex_destroy(&lock);
}

YuPollWaitStart
YuEpollLoop::beginWait(YuPollWaiter* waiter, uint64_t deadlineNanoseconds) {
    int fd = waiter->fd;
    bool isRead = waiter->filter == YuPollRead;
    YuEpollDescriptor* descriptor = ensureDescriptor(fd);
    if (!descriptor) {
        return YuPollWaitStartReady;
    }

    pthread_mutex_lock(&lock);
    if (descriptorLocked(fd) != descriptor || descriptor->unpollable) {
        pthread_mutex_unlock(&lock);
        return YuPollWaitStartReady;
    }

    bool& ready = isRead ? descriptor->readReady : descriptor->writeReady;
    if (ready) {
        ready = false;
        pthread_mutex_unlock(&lock);
        return YuPollWaitStartReady;
    }

    linkWaiterLocked(isRead ? descriptor->readers : descriptor->writers, waiter);
    bool wakes = false;
    if (deadlineNanoseconds) {
        waiter->timer.deadlineNanoseconds = deadlineNanoseconds;
        wakes = addTimerLocked(&waiter->timer);
    }
    bool needsRegistration = !descriptor->registered;
    descriptor->registered = true;
    pthread_mutex_unlock(&lock);

    if (wakes) {
        wakeLoop();
    }
    if (!needsRegistration) {
        return YuPollWaitStartSuspend;
    }

    uint32_t events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    int error = watch(fd, events, eventTag(kYuDescriptorEvent, fd));
    if (!error) {
        return YuPollWaitStartSuspend;
    }

    std::vector<YuTask*> tasks;
    pthread_mutex_lock(&lock);
    unlinkWaiterLocked(waiter);
    if (descriptorLocked(fd) == descriptor) {
        descriptor->registered = false;
        descriptor->unpollable = error == EPERM;
        takeWaitersLocked(descriptor->readers, tasks);
        takeWaitersLocked(descriptor->writers, tasks);
    }
    pthread_mutex_unlock(&lock);
    yumiScheduleTasks(tasks);
    return YuPollWaitStartReady;
}

void
YuEpollLoop::endWait(YuPollWaiter* waiter) {
    pthread_mutex_lock(&lock);
    unlinkWaiterLocked(waiter);
    pthread_mutex_unlock(&lock);
}

void
YuEpollLoop::releaseDescriptor(int fd) {
    std::vector<YuTask*> tasks;

    pthread_mutex_lock(&lock);
    YuEpollDescriptor* descriptor = descriptorLocked(fd);
    if (descriptor) {
        if (descriptor->registered) {
            epoll_ctl(epoll, EPOLL_CTL_DEL, fd, NULL);
        }
        takeWaitersLocked(descriptor->readers, tasks);
        takeWaitersLocked(descriptor->writers, tasks);
        descriptors[fd] = NULL;
        delete descriptor;
    }
    pthread_mutex_unlock(&lock);

    yumiScheduleTasks(tasks);
}

// -------------------------------------------------------------------------------------------------

void
YuEpollLoop::onProcExit(int pid, YuTask* task) {
    pthread_mutex_lock(&lock);
    auto found = processWatches.find(pid);
    if (found != processWatches.end()) {
        found->second.task = task;
        pthread_mutex_unlock(&lock);
        return;
    }

    int descriptor = static_cast<int>(syscall(SYS_pidfd_open, pid, 0));
    bool watching = descriptor >= 0;
    if (watching) {
        YuProcessWatch& process = processWatches[pid];
        process.descriptor = descriptor;
        process.task = task;
        if (watch(descriptor, EPOLLIN, eventTag(kYuProcessEvent, pid)) != 0) {
            processWatches.erase(pid);
            ::close(descriptor);
            watching = false;
        }
    }
    pthread_mutex_unlock(&lock);

    if (!watching) {
        task->getTaskQueue()->scheduleTask(task);
    }
}

void
YuEpollLoop::onFileChange(int, int, YuTask*) {
}

void
YuEpollLoop::onSignal(int sig, YuTask* task) {
    pthread_mutex_lock(&lock);
    signalWatches[sig] = task;
    pthread_mutex_unlock(&lock);

    struct sigaction action = {};
    action.sa_handler = yumiSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(sig, &action, NULL);
}

void
YuEpollLoop::onTimer(uint64_t deadlineNanoseconds, uint64_t id, YuTask* task) {
    YuTimerNode* node = new YuTimerNode();
    node->deadlineNanoseconds = deadlineNanoseconds;
    node->task = task;
    node->id = id;

    pthread_mutex_lock(&lock);
    sleepTimers[id] = node;
    bool wakes = addTimerLocked(node);
    pthread_mutex_unlock(&lock);

    if (wakes) {
        wakeLoop();
    }
}

void
YuEpollLoop::cancelProcExit(int pid) {
    pthread_mutex_lock(&lock);
    auto found = processWatches.find(pid);
    if (found != processWatches.end()) {
        ::close(found->second.descriptor);
        processWatches.erase(found);
    }
    pthread_mutex_unlock(&lock);
}

void
YuEpollLoop::cancelFileChange(int, int) {
}

void
YuEpollLoop::cancelSignal(int sig) {
    pthread_mutex_lock(&lock);
    signalWatches.erase(sig);
    pthread_mutex_unlock(&lock);
}

void
YuEpollLoop::cancelTimer(uint64_t id) {
    pthread_mutex_lock(&lock);
    auto found = sleepTimers.find(id);
    if (found != sleepTimers.end()) {
        YuTimerNode* node = found->second;
        sleepTimers.erase(found);
        timers.remove(node);
        delete node;
    }
    pthread_mutex_unlock(&lock);
}

uint64_t
YuEpollLoop::nextScheduledDeadlineNanoseconds() {
    pthread_mutex_lock(&lock);
    uint64_t deadline = timers.empty() ? 0 : timers.top()->deadlineNanoseconds;
    pthread_mutex_unlock(&lock);
    return deadline;
}

// -------------------------------------------------------------------------------------------------

void
YuEpollLoop::start() {
    epoll = epoll_create1(EPOLL_CLOEXEC);
    if (epoll < 0) {
        failLoop("epoll_create1");
    }
    wakeDescriptor = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (wakeDescriptor < 0) {
        failLoop("eventfd");
    }
    timerDescriptor = timerfd_create(CLOCK_BOOTTIME, TFD_CLOEXEC | TFD_NONBLOCK);
    if (timerDescriptor < 0) {
        failLoop("timerfd_create");
    }
    int signalPipe[2];
    if (pipe2(signalPipe, O_CLOEXEC | O_NONBLOCK) != 0) {
        failLoop("pipe2");
    }
    signalDescriptor = signalPipe[0];
    gSignalWriteDescriptor.store(signalPipe[1], std::memory_order_relaxed);

    if (watch(wakeDescriptor, EPOLLIN, eventTag(kYuWakeEvent, 0)) != 0 ||
        watch(timerDescriptor, EPOLLIN, eventTag(kYuTimerEvent, 0)) != 0 ||
        watch(signalDescriptor, EPOLLIN, eventTag(kYuSignalEvent, 0)) != 0) {
        failLoop("epoll_ctl");
    }

    int err = pthread_create(&tid, 0, YuEpollLoopThread, (void*)this);
    if (err) {
        printf("Unable to create thread (%d)\n", err);
        exit(-1);
    }
}

void
YuEpollLoop::stop() {
    stopRequested.store(true, std::memory_order_release);
    wakeLoop();
}

void
YuEpollLoop::join() {
    pthread_join(tid, NULL);
    ::close(gSignalWriteDescriptor.exchange(-1, std::memory_order_relaxed));
    ::close(signalDescriptor);
    ::close(timerDescriptor);
    ::close(wakeDescriptor);
    ::close(epoll);
    signalDescriptor = -1;
    timerDescriptor = -1;
    wakeDescriptor = -1;
    epoll = -1;
}

void
YuEpollLoop::runEventLoop() {
    struct epoll_event events[kYuEventBatchSize];
    std::vector<YuTask*> tasks;
    bool stopping = false;

    while (!stopping) {
        pthread_mutex_lock(&lock);
        uint64_t now = yumiMonotonicTimeNanoseconds();
        expireTimersLocked(now, tasks);
        uint64_t deadline = timers.empty() ? UINT64_MAX : timers.top()->deadlineNanoseconds;
        armedDeadlineNanoseconds = deadline;
        pthread_mutex_unlock(&lock);
        yumiScheduleTasks(tasks);
        armTimer(deadline);

        int count = epoll_wait(epoll, events, kYuEventBatchSize, -1);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            failLoop("epoll_wait");
        }

        pthread_mutex_lock(&lock);
        for (int i = 0; i < count; ++i) {
            uint64_t tag = events[i].data.u64;
            int value = static_cast<int>(static_cast<uint32_t>(tag));
            switch (tag >> 32) {
            case kYuDescriptorEvent:
                descriptorReadyLocked(value, events[i].events, tasks);
                break;
            case kYuWakeEvent:
                drainDescriptor(wakeDescriptor);
                stopping = stopRequested.load(std::memory_order_acquire);
                break;
            case kYuTimerEvent:
                drainDescriptor(timerDescriptor);
                timerDeadlineNanoseconds = UINT64_MAX;
                break;
            case kYuSignalEvent:
                signalsReadyLocked(tasks);
                break;
            case kYuProcessEvent:
                processExitedLocked(value, tasks);
                break;
            }
        }
        pthread_mutex_unlock(&lock);
        yumiScheduleTasks(tasks);
    }
}

// -------------------------------------------------------------------------------------------------

YuEpollDescriptor*
YuEpollLoop::descriptorLocked(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= descriptors.size()) {
        return NULL;
    }
    return descriptors[fd];
}

YuEpollDescriptor*
YuEpollLoop::ensureDescriptor(int fd) {
    if (fd < 0) {
        return NULL;
    }

    pthread_mutex_lock(&lock);
    YuEpollDescriptor* descriptor = descriptorLocked(fd);
    if (!descriptor) {
        descriptor = new YuEpollDescriptor();
        if (static_cast<size_t>(fd) >= descriptors.size()) {
            descriptors.resize(static_cast<size_t>(fd) + 1, NULL);
        }
        descriptors[fd] = descriptor;
    }
    pthread_mutex_unlock(&lock);
    return descriptor;
}

void
YuEpollLoop::linkWaiterLocked(YuPollWaiterList& list, YuPollWaiter* waiter) {
    waiter->prev = list.tail;
    waiter->next = NULL;
    if (list.tail) {
        list.tail->next = waiter;
    } else {
        list.head = waiter;
    }
    list.tail = waiter;
    waiter->linked = true;
}

void
YuEpollLoop::unlinkWaiterLocked(YuPollWaiter* waiter) {
    timers.remove(&waiter->timer);
    if (!waiter->linked) {
        return;
    }

    YuEpollDescriptor* descriptor = descriptorLocked(waiter->fd);
    if (descriptor) {
        YuPollWaiterList& list = waiter->filter == YuPollRead
            ? descriptor->readers
            : descriptor->writers;
        if (waiter->prev) {
            waiter->prev->next = waiter->next;
        } else {
            list.head = waiter->next;
        }
        if (waiter->next) {
            waiter->next->prev = waiter->prev;
        } else {
            list.tail = waiter->prev;
        }
    }
    waiter->prev = NULL;
    waiter->next = NULL;
    waiter->linked = false;
}

void
YuEpollLoop::takeWaitersLocked(YuPollWaiterList& list, std::vector<YuTask*>& tasks) {
    YuPollWaiter* waiter = list.head;
    list.head = NULL;
    list.tail = NULL;
    while (waiter) {
        YuPollWaiter* next = waiter->next;
        timers.remove(&waiter->timer);
        waiter->prev = NULL;
        waiter->next = NULL;
        waiter->linked = false;
        tasks.push_back(waiter->task);
        waiter = next;
    }
}

void
YuEpollLoop::descriptorReadyLocked(int fd, uint32_t events, std::vector<YuTask*>& tasks) {
    YuEpollDescriptor* descriptor = descriptorLocked(fd);
    if (!descriptor) {
        return;
    }

    if (events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        if (descriptor->readers.head) {
            takeWaitersLocked(descriptor->readers, tasks);
        } else {
            descriptor->readReady = true;
        }
    }
    if (events & (EPOLLOUT | EPOLLHUP | EPOLLERR)) {
        if (descriptor->writers.head) {
            takeWaitersLocked(descriptor->writers, tasks);
        } else {
            descriptor->writeReady = true;
        }
    }
}

void
YuEpollLoop::processExitedLocked(int pid, std::vector<YuTask*>& tasks) {
    auto found = processWatches.find(pid);
    if (found == processWatches.end()) {
        return;
    }
    ::close(found->second.descriptor);
    tasks.push_back(found->second.task);
    processWatches.erase(found);
}

void
YuEpollLoop::signalsReadyLocked(std::vector<YuTask*>& tasks) {
    unsigned char received[64];
    ssize_t count;
    while ((count = read(signalDescriptor, received, sizeof(received))) > 0) {
        for (ssize_t i = 0; i < count; ++i) {
            auto found = signalWatches.find(received[i]);
            if (found != signalWatches.end()) {
                tasks.push_back(found->second);
                signalWatches.erase(found);
            }
        }
    }
}

void
YuEpollLoop::expireTimersLocked(uint64_t now, std::vector<YuTask*>& tasks) {
    while (!timers.empty() && timers.top()->deadlineNanoseconds <= now) {
        YuTimerNode* node = timers.top();
        timers.remove(node);
        if (node->waiter) {
            node->waiter->timedOut = true;
            unlinkWaiterLocked(node->waiter);
            tasks.push_back(node->task);
        } else {
            sleepTimers.erase(node->id);
            tasks.push_back(node->task);
            delete node;
        }
    }
}

bool
YuEpollLoop::addTimerLocked(YuTimerNode* node) {
    timers.add(node);
    return timers.top() == node && node->deadlineNanoseconds < armedDeadlineNanoseconds;
}

void
YuEpollLoop::armTimer(uint64_t deadlineNanoseconds) {
    if (deadlineNanoseconds == timerDeadlineNanoseconds) {
        return;
    }
    timerDeadlineNanoseconds = deadlineNanoseconds;

    struct itimerspec spec = {};
    if (deadlineNanoseconds != UINT64_MAX) {
        spec.it_value.tv_sec = static_cast<time_t>(deadlineNanoseconds / 1000000000ull);
        spec.it_value.tv_nsec = static_cast<long>(deadlineNanoseconds % 1000000000ull);
    }
    if (timerfd_settime(timerDescriptor, TFD_TIMER_ABSTIME, &spec, NULL) != 0) {
        failLoop("timerfd_settime");
    }
}

int
YuEpollLoop::watch(int fd, uint32_t events, uint64_t tag) {
    struct epoll_event event = {};
    event.events = events;
    event.data.u64 = tag;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &event) == 0) {
        return 0;
    }
    if (errno == EEXIST && epoll_ctl(epoll, EPOLL_CTL_MOD, fd, &event) == 0) {
        return 0;
    }
    return errno;
}

void
YuEpollLoop::wakeLoop() {
    uint64_t value = 1;
    ssize_t written = write(wakeDescriptor, &value, sizeof(value));
    (void)written;
}
