#include <BxEvents/BxEpollLoop.h>
#include <BxEvents/BxTask.h>
#include <BxEvents/BxTaskQueue.h>

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

static const uint64_t kBxDescriptorEvent = 0;
static const uint64_t kBxWakeEvent = 1;
static const uint64_t kBxTimerEvent = 2;
static const uint64_t kBxSignalEvent = 3;
static const uint64_t kBxProcessEvent = 4;
static const int kBxEventBatchSize = 128;

static std::atomic<int> gSignalWriteDescriptor(-1);

static void*
BxEpollLoopThread(void* input) {
    BxEpollLoop* loop = (BxEpollLoop*)input;
    loop->runEventLoop();

    pthread_exit(NULL);
}

static void
brixScheduleTasks(std::vector<BxTask*>& tasks) {
    for (BxTask* task : tasks) {
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
brixSignalHandler(int sig) {
    int error = errno;
    unsigned char value = static_cast<unsigned char>(sig);
    ssize_t written = write(gSignalWriteDescriptor.load(std::memory_order_relaxed), &value, 1);
    (void)written;
    errno = error;
}

// *************************************************************************************************

BxEpollLoop::BxEpollLoop():
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

BxEpollLoop::~BxEpollLoop() {
    for (BxEpollDescriptor* descriptor : descriptors) {
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

BxPollWaitStart
BxEpollLoop::beginWait(BxPollWaiter* waiter, uint64_t deadlineNanoseconds) {
    int fd = waiter->fd;
    bool isRead = waiter->filter == BxPollRead;
    BxEpollDescriptor* descriptor = ensureDescriptor(fd);
    if (!descriptor) {
        return BxPollWaitStartReady;
    }

    pthread_mutex_lock(&lock);
    if (descriptorLocked(fd) != descriptor || descriptor->unpollable) {
        pthread_mutex_unlock(&lock);
        return BxPollWaitStartReady;
    }

    bool& ready = isRead ? descriptor->readReady : descriptor->writeReady;
    if (ready) {
        ready = false;
        pthread_mutex_unlock(&lock);
        return BxPollWaitStartReady;
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
        return BxPollWaitStartSuspend;
    }

    uint32_t events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    int error = watch(fd, events, eventTag(kBxDescriptorEvent, fd));
    if (!error) {
        return BxPollWaitStartSuspend;
    }

    std::vector<BxTask*> tasks;
    pthread_mutex_lock(&lock);
    unlinkWaiterLocked(waiter);
    if (descriptorLocked(fd) == descriptor) {
        descriptor->registered = false;
        descriptor->unpollable = error == EPERM;
        takeWaitersLocked(descriptor->readers, tasks);
        takeWaitersLocked(descriptor->writers, tasks);
    }
    pthread_mutex_unlock(&lock);
    brixScheduleTasks(tasks);
    return BxPollWaitStartReady;
}

void
BxEpollLoop::endWait(BxPollWaiter* waiter) {
    pthread_mutex_lock(&lock);
    unlinkWaiterLocked(waiter);
    pthread_mutex_unlock(&lock);
}

void
BxEpollLoop::releaseDescriptor(int fd) {
    std::vector<BxTask*> tasks;

    pthread_mutex_lock(&lock);
    BxEpollDescriptor* descriptor = descriptorLocked(fd);
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

    brixScheduleTasks(tasks);
}

// -------------------------------------------------------------------------------------------------

void
BxEpollLoop::onProcExit(int pid, BxTask* task) {
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
        BxProcessWatch& process = processWatches[pid];
        process.descriptor = descriptor;
        process.task = task;
        if (watch(descriptor, EPOLLIN, eventTag(kBxProcessEvent, pid)) != 0) {
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
BxEpollLoop::onFileChange(int, int, BxTask*) {
}

void
BxEpollLoop::onSignal(int sig, BxTask* task) {
    pthread_mutex_lock(&lock);
    signalWatches[sig] = task;
    pthread_mutex_unlock(&lock);

    struct sigaction action = {};
    action.sa_handler = brixSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(sig, &action, NULL);
}

void
BxEpollLoop::onTimer(uint64_t deadlineNanoseconds, uint64_t id, BxTask* task) {
    BxTimerNode* node = new BxTimerNode();
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
BxEpollLoop::cancelProcExit(int pid) {
    pthread_mutex_lock(&lock);
    auto found = processWatches.find(pid);
    if (found != processWatches.end()) {
        ::close(found->second.descriptor);
        processWatches.erase(found);
    }
    pthread_mutex_unlock(&lock);
}

void
BxEpollLoop::cancelFileChange(int, int) {
}

void
BxEpollLoop::cancelSignal(int sig) {
    pthread_mutex_lock(&lock);
    signalWatches.erase(sig);
    pthread_mutex_unlock(&lock);
}

void
BxEpollLoop::cancelTimer(uint64_t id) {
    pthread_mutex_lock(&lock);
    auto found = sleepTimers.find(id);
    if (found != sleepTimers.end()) {
        BxTimerNode* node = found->second;
        sleepTimers.erase(found);
        timers.remove(node);
        delete node;
    }
    pthread_mutex_unlock(&lock);
}

uint64_t
BxEpollLoop::nextScheduledDeadlineNanoseconds() {
    pthread_mutex_lock(&lock);
    uint64_t deadline = timers.empty() ? 0 : timers.top()->deadlineNanoseconds;
    pthread_mutex_unlock(&lock);
    return deadline;
}

// -------------------------------------------------------------------------------------------------

void
BxEpollLoop::start() {
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

    if (watch(wakeDescriptor, EPOLLIN, eventTag(kBxWakeEvent, 0)) != 0 ||
        watch(timerDescriptor, EPOLLIN, eventTag(kBxTimerEvent, 0)) != 0 ||
        watch(signalDescriptor, EPOLLIN, eventTag(kBxSignalEvent, 0)) != 0) {
        failLoop("epoll_ctl");
    }

    int err = pthread_create(&tid, 0, BxEpollLoopThread, (void*)this);
    if (err) {
        printf("Unable to create thread (%d)\n", err);
        exit(-1);
    }
}

void
BxEpollLoop::stop() {
    stopRequested.store(true, std::memory_order_release);
    wakeLoop();
}

void
BxEpollLoop::join() {
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
BxEpollLoop::runEventLoop() {
    struct epoll_event events[kBxEventBatchSize];
    std::vector<BxTask*> tasks;
    bool stopping = false;

    while (!stopping) {
        pthread_mutex_lock(&lock);
        uint64_t now = brixMonotonicTimeNanoseconds();
        expireTimersLocked(now, tasks);
        uint64_t deadline = timers.empty() ? UINT64_MAX : timers.top()->deadlineNanoseconds;
        armedDeadlineNanoseconds = deadline;
        pthread_mutex_unlock(&lock);
        brixScheduleTasks(tasks);
        armTimer(deadline);

        int count = epoll_wait(epoll, events, kBxEventBatchSize, -1);
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
            case kBxDescriptorEvent:
                descriptorReadyLocked(value, events[i].events, tasks);
                break;
            case kBxWakeEvent:
                drainDescriptor(wakeDescriptor);
                stopping = stopRequested.load(std::memory_order_acquire);
                break;
            case kBxTimerEvent:
                drainDescriptor(timerDescriptor);
                timerDeadlineNanoseconds = UINT64_MAX;
                break;
            case kBxSignalEvent:
                signalsReadyLocked(tasks);
                break;
            case kBxProcessEvent:
                processExitedLocked(value, tasks);
                break;
            }
        }
        pthread_mutex_unlock(&lock);
        brixScheduleTasks(tasks);
    }
}

// -------------------------------------------------------------------------------------------------

BxEpollDescriptor*
BxEpollLoop::descriptorLocked(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= descriptors.size()) {
        return NULL;
    }
    return descriptors[fd];
}

BxEpollDescriptor*
BxEpollLoop::ensureDescriptor(int fd) {
    if (fd < 0) {
        return NULL;
    }

    pthread_mutex_lock(&lock);
    BxEpollDescriptor* descriptor = descriptorLocked(fd);
    if (!descriptor) {
        descriptor = new BxEpollDescriptor();
        if (static_cast<size_t>(fd) >= descriptors.size()) {
            descriptors.resize(static_cast<size_t>(fd) + 1, NULL);
        }
        descriptors[fd] = descriptor;
    }
    pthread_mutex_unlock(&lock);
    return descriptor;
}

void
BxEpollLoop::linkWaiterLocked(BxPollWaiterList& list, BxPollWaiter* waiter) {
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
BxEpollLoop::unlinkWaiterLocked(BxPollWaiter* waiter) {
    timers.remove(&waiter->timer);
    if (!waiter->linked) {
        return;
    }

    BxEpollDescriptor* descriptor = descriptorLocked(waiter->fd);
    if (descriptor) {
        BxPollWaiterList& list = waiter->filter == BxPollRead
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
BxEpollLoop::takeWaitersLocked(BxPollWaiterList& list, std::vector<BxTask*>& tasks) {
    BxPollWaiter* waiter = list.head;
    list.head = NULL;
    list.tail = NULL;
    while (waiter) {
        BxPollWaiter* next = waiter->next;
        timers.remove(&waiter->timer);
        waiter->prev = NULL;
        waiter->next = NULL;
        waiter->linked = false;
        tasks.push_back(waiter->task);
        waiter = next;
    }
}

void
BxEpollLoop::descriptorReadyLocked(int fd, uint32_t events, std::vector<BxTask*>& tasks) {
    BxEpollDescriptor* descriptor = descriptorLocked(fd);
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
BxEpollLoop::processExitedLocked(int pid, std::vector<BxTask*>& tasks) {
    auto found = processWatches.find(pid);
    if (found == processWatches.end()) {
        return;
    }
    ::close(found->second.descriptor);
    tasks.push_back(found->second.task);
    processWatches.erase(found);
}

void
BxEpollLoop::signalsReadyLocked(std::vector<BxTask*>& tasks) {
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
BxEpollLoop::expireTimersLocked(uint64_t now, std::vector<BxTask*>& tasks) {
    while (!timers.empty() && timers.top()->deadlineNanoseconds <= now) {
        BxTimerNode* node = timers.top();
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
BxEpollLoop::addTimerLocked(BxTimerNode* node) {
    timers.add(node);
    return timers.top() == node && node->deadlineNanoseconds < armedDeadlineNanoseconds;
}

void
BxEpollLoop::armTimer(uint64_t deadlineNanoseconds) {
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
BxEpollLoop::watch(int fd, uint32_t events, uint64_t tag) {
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
BxEpollLoop::wakeLoop() {
    uint64_t value = 1;
    ssize_t written = write(wakeDescriptor, &value, sizeof(value));
    (void)written;
}
