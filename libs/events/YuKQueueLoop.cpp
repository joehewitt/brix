
#include <YuEvents/YuKQueueLoop.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuTaskQueue.h>

#include <stdlib.h>
#include <cerrno>
#include <cstdio>
#include <unistd.h>

// *************************************************************************************************

static const uintptr_t kYuStopEvent = 1;
static const uintptr_t kYuWakeEvent = 2;
static const int kYuEventBatchSize = 128;

static void*
YuKQueueLoopThread(void* input) {
    YuKQueueLoop *loop = (YuKQueueLoop*)input;
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

static bool
yumiFilterNeedsSelect(int error) {
    return error == EINVAL || error == ENODEV || error == ENOTSUP;
}

// *************************************************************************************************

YuKQueueLoop::YuKQueueLoop():
    kq(-1),
    armedDeadlineNanoseconds(UINT64_MAX)
{
    pthread_mutex_init(&lock, 0);
}

YuKQueueLoop::~YuKQueueLoop() {
    for (YuPollDescriptor* descriptor : descriptors) {
        delete descriptor;
    }
    for (auto& entry : sleepTimers) {
        delete entry.second;
    }
    pthread_mutex_destroy(&lock);
}

YuPollWaitStart
YuKQueueLoop::beginWait(YuPollWaiter* waiter, uint64_t deadlineNanoseconds) {
    int fd = waiter->fd;
    bool isRead = waiter->filter == EVFILT_READ;
    YuPollDescriptor* descriptor = ensureDescriptor(fd);
    if (!descriptor) {
        return YuPollWaitStartReady;
    }

    pthread_mutex_lock(&lock);
    if (descriptorLocked(fd) != descriptor) {
        pthread_mutex_unlock(&lock);
        return YuPollWaitStartReady;
    }
    if (descriptor->usesSelect) {
        pthread_mutex_unlock(&lock);
        return YuPollWaitStartSelect;
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
    bool& registered = isRead ? descriptor->readRegistered : descriptor->writeRegistered;
    bool needsRegistration = !registered;
    registered = true;
    pthread_mutex_unlock(&lock);

    if (wakes) {
        wakeLoop();
    }
    if (!needsRegistration) {
        return YuPollWaitStartSuspend;
    }

    int error = registerFilter(fd, waiter->filter);
    if (!error) {
        return YuPollWaitStartSuspend;
    }

    YuPollWaitStart start = YuPollWaitStartReady;
    pthread_mutex_lock(&lock);
    unlinkWaiterLocked(waiter);
    if (descriptorLocked(fd) == descriptor) {
        (isRead ? descriptor->readRegistered : descriptor->writeRegistered) = false;
        if (yumiFilterNeedsSelect(error)) {
            descriptor->usesSelect = true;
            start = YuPollWaitStartSelect;
        }
    }
    pthread_mutex_unlock(&lock);
    return start;
}

void
YuKQueueLoop::endWait(YuPollWaiter* waiter) {
    pthread_mutex_lock(&lock);
    unlinkWaiterLocked(waiter);
    pthread_mutex_unlock(&lock);
}

bool
YuKQueueLoop::releaseDescriptor(int fd) {
    std::vector<YuTask*> tasks;
    bool usesSelect = false;

    pthread_mutex_lock(&lock);
    YuPollDescriptor* descriptor = descriptorLocked(fd);
    if (descriptor) {
        usesSelect = descriptor->usesSelect;
        takeWaitersLocked(descriptor->readers, tasks);
        takeWaitersLocked(descriptor->writers, tasks);
        descriptors[fd] = NULL;
        delete descriptor;
    }
    pthread_mutex_unlock(&lock);

    yumiScheduleTasks(tasks);
    return usesSelect;
}

// -------------------------------------------------------------------------------------------------

void
YuKQueueLoop::onProcExit(int pid, YuTask* task) {
    struct kevent change;
    EV_SET(&change, pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT | NOTE_EXITSTATUS, 0, task);
    if (kevent(kq, &change, 1, NULL, 0, NULL) < 0 && errno == ESRCH) {
        task->getTaskQueue()->scheduleTask(task);
    }
}

void
YuKQueueLoop::onFileChange(int fd, int type, YuTask* task) {
    struct kevent change;
    EV_SET(&change, fd, EVFILT_VNODE, EV_ADD | EV_ONESHOT, type, 0, task);
    submit(change);
}

void
YuKQueueLoop::onSignal(int sig, YuTask* task) {
    struct kevent change;
    EV_SET(&change, sig, EVFILT_SIGNAL, EV_ADD | EV_ONESHOT, 0, 0, task);
    submit(change);
}

void
YuKQueueLoop::onTimer(uint64_t deadlineNanoseconds, uint64_t id, YuTask* task) {
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
YuKQueueLoop::cancelProcExit(int pid) {
    struct kevent change;
    EV_SET(&change, pid, EVFILT_PROC, EV_DELETE, 0, 0, NULL);
    submit(change);
}

void
YuKQueueLoop::cancelFileChange(int fd, int type) {
    struct kevent change;
    EV_SET(&change, fd, EVFILT_VNODE, EV_DELETE, type, 0, NULL);
    submit(change);
}

void
YuKQueueLoop::cancelSignal(int sig) {
    struct kevent change;
    EV_SET(&change, sig, EVFILT_SIGNAL, EV_DELETE, 0, 0, NULL);
    submit(change);
}

void
YuKQueueLoop::cancelTimer(uint64_t id) {
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
YuKQueueLoop::nextScheduledDeadlineNanoseconds() {
    pthread_mutex_lock(&lock);
    uint64_t deadline = timers.empty() ? 0 : timers.top()->deadlineNanoseconds;
    pthread_mutex_unlock(&lock);
    return deadline;
}

// -------------------------------------------------------------------------------------------------

void
YuKQueueLoop::start() {
    kq = kqueue();
    if (kq < 0) {
        perror("kqueue");
        abort();
    }

    struct kevent changes[2];
    EV_SET(&changes[0], kYuStopEvent, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, NULL);
    EV_SET(&changes[1], kYuWakeEvent, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, NULL);
    if (kevent(kq, changes, 2, NULL, 0, NULL) < 0) {
        perror("kevent");
        abort();
    }

    int err = pthread_create(&tid, 0, YuKQueueLoopThread, (void*)this);
    if (err) {
        printf("Unable to create thread (%d)\n", err);
        exit(-1);
    }
}

void
YuKQueueLoop::stop() {
    struct kevent change;
    EV_SET(&change, kYuStopEvent, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
    submit(change);
}

void
YuKQueueLoop::join() {
    pthread_join(tid, NULL);
    ::close(kq);
    kq = -1;
}

void
YuKQueueLoop::runEventLoop() {
    struct kevent events[kYuEventBatchSize];
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

        struct timespec timeout;
        struct timespec* timeoutPointer = NULL;
        if (deadline != UINT64_MAX) {
            uint64_t delay = deadline > now ? deadline - now : 0;
            timeout.tv_sec = static_cast<time_t>(delay / 1000000000ull);
            timeout.tv_nsec = static_cast<long>(delay % 1000000000ull);
            timeoutPointer = &timeout;
        }

        int count = kevent(kq, NULL, 0, events, kYuEventBatchSize, timeoutPointer);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("kevent");
            abort();
        }

        pthread_mutex_lock(&lock);
        for (int i = 0; i < count; ++i) {
            struct kevent& event = events[i];
            if (event.filter == EVFILT_USER) {
                if (event.ident == kYuStopEvent) {
                    stopping = true;
                }
            } else if (event.filter == EVFILT_READ || event.filter == EVFILT_WRITE) {
                descriptorReadyLocked(static_cast<int>(event.ident), event.filter, tasks);
            } else if (event.udata) {
                tasks.push_back(static_cast<YuTask*>(event.udata));
            }
        }
        pthread_mutex_unlock(&lock);
        yumiScheduleTasks(tasks);
    }
}

// -------------------------------------------------------------------------------------------------

YuPollDescriptor*
YuKQueueLoop::descriptorLocked(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= descriptors.size()) {
        return NULL;
    }
    return descriptors[fd];
}

YuPollDescriptor*
YuKQueueLoop::ensureDescriptor(int fd) {
    pthread_mutex_lock(&lock);
    YuPollDescriptor* descriptor = descriptorLocked(fd);
    pthread_mutex_unlock(&lock);
    if (descriptor || fd < 0) {
        return descriptor;
    }

    bool isTTY = isatty(fd);

    pthread_mutex_lock(&lock);
    descriptor = descriptorLocked(fd);
    if (!descriptor) {
        descriptor = new YuPollDescriptor();
        descriptor->usesSelect = isTTY;
        if (static_cast<size_t>(fd) >= descriptors.size()) {
            descriptors.resize(static_cast<size_t>(fd) + 1, NULL);
        }
        descriptors[fd] = descriptor;
    }
    pthread_mutex_unlock(&lock);
    return descriptor;
}

void
YuKQueueLoop::linkWaiterLocked(YuPollWaiterList& list, YuPollWaiter* waiter) {
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
YuKQueueLoop::unlinkWaiterLocked(YuPollWaiter* waiter) {
    timers.remove(&waiter->timer);
    if (!waiter->linked) {
        return;
    }

    YuPollDescriptor* descriptor = descriptorLocked(waiter->fd);
    if (descriptor) {
        YuPollWaiterList& list = waiter->filter == EVFILT_READ
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
YuKQueueLoop::takeWaitersLocked(YuPollWaiterList& list, std::vector<YuTask*>& tasks) {
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
YuKQueueLoop::descriptorReadyLocked(int fd, int16_t filter, std::vector<YuTask*>& tasks) {
    YuPollDescriptor* descriptor = descriptorLocked(fd);
    if (!descriptor) {
        return;
    }

    bool isRead = filter == EVFILT_READ;
    YuPollWaiterList& list = isRead ? descriptor->readers : descriptor->writers;
    if (list.head) {
        takeWaitersLocked(list, tasks);
    } else if (isRead) {
        descriptor->readReady = true;
    } else {
        descriptor->writeReady = true;
    }
}

void
YuKQueueLoop::expireTimersLocked(uint64_t now, std::vector<YuTask*>& tasks) {
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
YuKQueueLoop::addTimerLocked(YuTimerNode* node) {
    timers.add(node);
    return timers.top() == node && node->deadlineNanoseconds < armedDeadlineNanoseconds;
}

int
YuKQueueLoop::registerFilter(int fd, int16_t filter) {
    struct kevent change;
    EV_SET(&change, fd, filter, EV_ADD | EV_CLEAR, 0, 0, NULL);
    return kevent(kq, &change, 1, NULL, 0, NULL) < 0 ? errno : 0;
}

void
YuKQueueLoop::submit(struct kevent& change) {
    kevent(kq, &change, 1, NULL, 0, NULL);
}

void
YuKQueueLoop::wakeLoop() {
    struct kevent change;
    EV_SET(&change, kYuWakeEvent, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
    submit(change);
}
