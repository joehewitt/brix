
#include <BxEvents/BxKQueueLoop.h>
#include <BxEvents/BxTask.h>
#include <BxEvents/BxTaskQueue.h>

#include <stdlib.h>
#include <cerrno>
#include <cstdio>
#include <unistd.h>

// *************************************************************************************************

static const uintptr_t kBxStopEvent = 1;
static const uintptr_t kBxWakeEvent = 2;
static const int kBxEventBatchSize = 128;

static void*
BxKQueueLoopThread(void* input) {
    BxKQueueLoop *loop = (BxKQueueLoop*)input;
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

static bool
brixFilterNeedsSelect(int error) {
    return error == EINVAL || error == ENODEV || error == ENOTSUP;
}

// *************************************************************************************************

BxKQueueLoop::BxKQueueLoop():
    kq(-1),
    armedDeadlineNanoseconds(UINT64_MAX)
{
    pthread_mutex_init(&lock, 0);
}

BxKQueueLoop::~BxKQueueLoop() {
    for (BxPollDescriptor* descriptor : descriptors) {
        delete descriptor;
    }
    for (auto& entry : sleepTimers) {
        delete entry.second;
    }
    pthread_mutex_destroy(&lock);
}

BxPollWaitStart
BxKQueueLoop::beginWait(BxPollWaiter* waiter, uint64_t deadlineNanoseconds) {
    int fd = waiter->fd;
    bool isRead = waiter->filter == EVFILT_READ;
    BxPollDescriptor* descriptor = ensureDescriptor(fd);
    if (!descriptor) {
        return BxPollWaitStartReady;
    }

    pthread_mutex_lock(&lock);
    if (descriptorLocked(fd) != descriptor) {
        pthread_mutex_unlock(&lock);
        return BxPollWaitStartReady;
    }
    if (descriptor->usesSelect) {
        pthread_mutex_unlock(&lock);
        return BxPollWaitStartSelect;
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
    bool& registered = isRead ? descriptor->readRegistered : descriptor->writeRegistered;
    bool needsRegistration = !registered;
    registered = true;
    pthread_mutex_unlock(&lock);

    if (wakes) {
        wakeLoop();
    }
    if (!needsRegistration) {
        return BxPollWaitStartSuspend;
    }

    int error = registerFilter(fd, waiter->filter);
    if (!error) {
        return BxPollWaitStartSuspend;
    }

    BxPollWaitStart start = BxPollWaitStartReady;
    pthread_mutex_lock(&lock);
    unlinkWaiterLocked(waiter);
    if (descriptorLocked(fd) == descriptor) {
        (isRead ? descriptor->readRegistered : descriptor->writeRegistered) = false;
        if (brixFilterNeedsSelect(error)) {
            descriptor->usesSelect = true;
            start = BxPollWaitStartSelect;
        }
    }
    pthread_mutex_unlock(&lock);
    return start;
}

void
BxKQueueLoop::endWait(BxPollWaiter* waiter) {
    pthread_mutex_lock(&lock);
    unlinkWaiterLocked(waiter);
    pthread_mutex_unlock(&lock);
}

bool
BxKQueueLoop::releaseDescriptor(int fd) {
    std::vector<BxTask*> tasks;
    bool usesSelect = false;

    pthread_mutex_lock(&lock);
    BxPollDescriptor* descriptor = descriptorLocked(fd);
    if (descriptor) {
        usesSelect = descriptor->usesSelect;
        takeWaitersLocked(descriptor->readers, tasks);
        takeWaitersLocked(descriptor->writers, tasks);
        descriptors[fd] = NULL;
        delete descriptor;
    }
    pthread_mutex_unlock(&lock);

    brixScheduleTasks(tasks);
    return usesSelect;
}

// -------------------------------------------------------------------------------------------------

bool
BxKQueueLoop::watchProcExit(int pid) {
    struct kevent change;
    EV_SET(&change, pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, NULL);
    return kevent(kq, &change, 1, NULL, 0, NULL) == 0 || errno != ESRCH;
}

void
BxKQueueLoop::onFileChange(int fd, int type, BxTask* task) {
    struct kevent change;
    EV_SET(&change, fd, EVFILT_VNODE, EV_ADD | EV_ONESHOT, type, 0, task);
    submit(change);
}

void
BxKQueueLoop::onSignal(int sig, BxTask* task) {
    struct kevent change;
    EV_SET(&change, sig, EVFILT_SIGNAL, EV_ADD | EV_ONESHOT, 0, 0, task);
    submit(change);
}

void
BxKQueueLoop::onTimer(uint64_t deadlineNanoseconds, uint64_t id, BxTask* task) {
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
BxKQueueLoop::unwatchProcExit(int pid) {
    struct kevent change;
    EV_SET(&change, pid, EVFILT_PROC, EV_DELETE, 0, 0, NULL);
    submit(change);
}

void
BxKQueueLoop::cancelFileChange(int fd, int type) {
    struct kevent change;
    EV_SET(&change, fd, EVFILT_VNODE, EV_DELETE, type, 0, NULL);
    submit(change);
}

void
BxKQueueLoop::cancelSignal(int sig) {
    struct kevent change;
    EV_SET(&change, sig, EVFILT_SIGNAL, EV_DELETE, 0, 0, NULL);
    submit(change);
}

void
BxKQueueLoop::cancelTimer(uint64_t id) {
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
BxKQueueLoop::nextScheduledDeadlineNanoseconds() {
    pthread_mutex_lock(&lock);
    uint64_t deadline = timers.empty() ? 0 : timers.top()->deadlineNanoseconds;
    pthread_mutex_unlock(&lock);
    return deadline;
}

// -------------------------------------------------------------------------------------------------

void
BxKQueueLoop::start() {
    kq = kqueue();
    if (kq < 0) {
        perror("kqueue");
        abort();
    }

    struct kevent changes[2];
    EV_SET(&changes[0], kBxStopEvent, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, NULL);
    EV_SET(&changes[1], kBxWakeEvent, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, NULL);
    if (kevent(kq, changes, 2, NULL, 0, NULL) < 0) {
        perror("kevent");
        abort();
    }

    int err = pthread_create(&tid, 0, BxKQueueLoopThread, (void*)this);
    if (err) {
        printf("Unable to create thread (%d)\n", err);
        exit(-1);
    }
}

void
BxKQueueLoop::stop() {
    struct kevent change;
    EV_SET(&change, kBxStopEvent, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
    submit(change);
}

void
BxKQueueLoop::join() {
    pthread_join(tid, NULL);
    ::close(kq);
    kq = -1;
}

void
BxKQueueLoop::runEventLoop() {
    struct kevent events[kBxEventBatchSize];
    std::vector<BxTask*> tasks;
    std::vector<int> exited;
    bool stopping = false;

    while (!stopping) {
        pthread_mutex_lock(&lock);
        uint64_t now = brixMonotonicTimeNanoseconds();
        expireTimersLocked(now, tasks);
        uint64_t deadline = timers.empty() ? UINT64_MAX : timers.top()->deadlineNanoseconds;
        armedDeadlineNanoseconds = deadline;
        pthread_mutex_unlock(&lock);
        brixScheduleTasks(tasks);

        struct timespec timeout;
        struct timespec* timeoutPointer = NULL;
        if (deadline != UINT64_MAX) {
            uint64_t delay = deadline > now ? deadline - now : 0;
            timeout.tv_sec = static_cast<time_t>(delay / 1000000000ull);
            timeout.tv_nsec = static_cast<long>(delay % 1000000000ull);
            timeoutPointer = &timeout;
        }

        int count = kevent(kq, NULL, 0, events, kBxEventBatchSize, timeoutPointer);
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
                if (event.ident == kBxStopEvent) {
                    stopping = true;
                }
            } else if (event.filter == EVFILT_READ || event.filter == EVFILT_WRITE) {
                descriptorReadyLocked(static_cast<int>(event.ident), event.filter, tasks);
            } else if (event.filter == EVFILT_PROC) {
                exited.push_back(static_cast<int>(event.ident));
            } else if (event.udata) {
                tasks.push_back(static_cast<BxTask*>(event.udata));
            }
        }
        pthread_mutex_unlock(&lock);
        brixScheduleTasks(tasks);
        if (!exited.empty()) {
            for (int pid : exited) {
                brixProcessExited(pid);
            }
            exited.clear();
        }
    }
}

// -------------------------------------------------------------------------------------------------

BxPollDescriptor*
BxKQueueLoop::descriptorLocked(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= descriptors.size()) {
        return NULL;
    }
    return descriptors[fd];
}

BxPollDescriptor*
BxKQueueLoop::ensureDescriptor(int fd) {
    pthread_mutex_lock(&lock);
    BxPollDescriptor* descriptor = descriptorLocked(fd);
    pthread_mutex_unlock(&lock);
    if (descriptor || fd < 0) {
        return descriptor;
    }

    bool isTTY = isatty(fd);

    pthread_mutex_lock(&lock);
    descriptor = descriptorLocked(fd);
    if (!descriptor) {
        descriptor = new BxPollDescriptor();
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
BxKQueueLoop::linkWaiterLocked(BxPollWaiterList& list, BxPollWaiter* waiter) {
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
BxKQueueLoop::unlinkWaiterLocked(BxPollWaiter* waiter) {
    timers.remove(&waiter->timer);
    if (!waiter->linked) {
        return;
    }

    BxPollDescriptor* descriptor = descriptorLocked(waiter->fd);
    if (descriptor) {
        BxPollWaiterList& list = waiter->filter == EVFILT_READ
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
BxKQueueLoop::takeWaitersLocked(BxPollWaiterList& list, std::vector<BxTask*>& tasks) {
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
BxKQueueLoop::descriptorReadyLocked(int fd, int16_t filter, std::vector<BxTask*>& tasks) {
    BxPollDescriptor* descriptor = descriptorLocked(fd);
    if (!descriptor) {
        return;
    }

    bool isRead = filter == EVFILT_READ;
    BxPollWaiterList& list = isRead ? descriptor->readers : descriptor->writers;
    if (list.head) {
        takeWaitersLocked(list, tasks);
    } else if (isRead) {
        descriptor->readReady = true;
    } else {
        descriptor->writeReady = true;
    }
}

void
BxKQueueLoop::expireTimersLocked(uint64_t now, std::vector<BxTask*>& tasks) {
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
BxKQueueLoop::addTimerLocked(BxTimerNode* node) {
    timers.add(node);
    return timers.top() == node && node->deadlineNanoseconds < armedDeadlineNanoseconds;
}

int
BxKQueueLoop::registerFilter(int fd, int16_t filter) {
    struct kevent change;
    EV_SET(&change, fd, filter, EV_ADD | EV_CLEAR, 0, 0, NULL);
    return kevent(kq, &change, 1, NULL, 0, NULL) < 0 ? errno : 0;
}

void
BxKQueueLoop::submit(struct kevent& change) {
    kevent(kq, &change, 1, NULL, 0, NULL);
}

void
BxKQueueLoop::wakeLoop() {
    struct kevent change;
    EV_SET(&change, kBxWakeEvent, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
    submit(change);
}
