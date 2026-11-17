#include <BxEvents/BxSelectLoop.h>
#include <BxEvents/BxTaskQueue.h>
#include <BxEvents/BxTask.h>

#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// *************************************************************************************************

static void*
BxSelectLoopThread(void* input) {
    BxSelectLoop *loop = (BxSelectLoop*)input;
    loop->runEventLoop();

    pthread_exit(NULL);
}

static void
BxSelectScheduleTasks(std::vector<BxTask*>& tasks) {
    for (BxTask* task : tasks) {
        task->getTaskQueue()->scheduleTask(task);
    }
}

static struct timeval
BxSelectTimeout(uint64_t deadlineNanoseconds) {
    struct timeval timeout;
    if (!deadlineNanoseconds) {
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;
        return timeout;
    }

    uint64_t now = brixMonotonicTimeNanoseconds();
    uint64_t remaining = deadlineNanoseconds > now ? deadlineNanoseconds - now : 0;
    uint64_t microseconds = (remaining + 999) / 1000;
    timeout.tv_sec = microseconds / 1000000;
    timeout.tv_usec = microseconds % 1000000;
    return timeout;
}

// *************************************************************************************************

BxSelectLoop::BxSelectLoop() :
    isStopped(false)
{
    pthread_mutex_init(&lock, 0);
}

BxSelectLoop::~BxSelectLoop() {
    pthread_mutex_destroy(&lock);
}

void
BxSelectLoop::beginWait(BxSelectWaiter* waiter) {
    pthread_mutex_lock(&lock);
    waiters.push_back(waiter);
    pthread_mutex_unlock(&lock);

    trigger.trigger();
}

void
BxSelectLoop::endWait(BxSelectWaiter* waiter) {
    pthread_mutex_lock(&lock);
    for (size_t i = 0; i < waiters.size(); ++i) {
        if (waiters[i] == waiter) {
            waiters.erase(waiters.begin() + i);
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

void
BxSelectLoop::cancelDescriptor(int fd) {
    std::vector<BxTask*> tasks;
    pthread_mutex_lock(&lock);
    for (size_t i = waiters.size(); i-- > 0;) {
        if (waiters[i]->fd == fd) {
            tasks.push_back(waiters[i]->task);
            waiters.erase(waiters.begin() + i);
        }
    }
    pthread_mutex_unlock(&lock);

    BxSelectScheduleTasks(tasks);
    trigger.trigger();
}

// *************************************************************************************************

int
BxSelectLoop::prepareHandlesLocked(fd_set* readerHandles, fd_set* writerHandles,
    uint64_t& deadlineNanoseconds) {
    FD_ZERO(readerHandles);
    FD_ZERO(writerHandles);
    FD_SET(trigger.fd, readerHandles);
    int fdMax = trigger.fd;
    deadlineNanoseconds = 0;

    for (BxSelectWaiter* waiter : waiters) {
        FD_SET(waiter->fd, waiter->isRead ? readerHandles : writerHandles);
        if (waiter->fd > fdMax) {
            fdMax = waiter->fd;
        }
        uint64_t deadline = waiter->deadlineNanoseconds;
        if (deadline && (!deadlineNanoseconds || deadline < deadlineNanoseconds)) {
            deadlineNanoseconds = deadline;
        }
    }
    return fdMax;
}

void
BxSelectLoop::takeReadyLocked(fd_set* readerHandles, fd_set* writerHandles, uint64_t now,
    std::vector<BxTask*>& tasks) {
    for (size_t i = waiters.size(); i-- > 0;) {
        BxSelectWaiter* waiter = waiters[i];
        bool isReady = FD_ISSET(waiter->fd, waiter->isRead ? readerHandles : writerHandles);
        bool hasExpired = !isReady && waiter->deadlineNanoseconds &&
            waiter->deadlineNanoseconds <= now;
        if (isReady || hasExpired) {
            waiter->timedOut = hasExpired;
            tasks.push_back(waiter->task);
            waiters.erase(waiters.begin() + i);
        }
    }
}

void
BxSelectLoop::takeInvalidLocked(std::vector<BxTask*>& tasks) {
    for (size_t i = waiters.size(); i-- > 0;) {
        if (fcntl(waiters[i]->fd, F_GETFD) == -1) {
            tasks.push_back(waiters[i]->task);
            waiters.erase(waiters.begin() + i);
        }
    }
}

// *************************************************************************************************

void
BxSelectLoop::start() {
    isStopped = false;
    int err = pthread_create(&tid, 0, BxSelectLoopThread, (void*)this);
    if (err) {
        printf("Unable to create thread (%d)\n", err);
        exit(-1);
    }
}

void
BxSelectLoop::stop() {
    isStopped = true;
    trigger.trigger();
}

void
BxSelectLoop::join() {
    pthread_join(tid, NULL);
}

// --------------------------------------------------------------------------------------------------

void
BxSelectLoop::runEventLoop() {
    while (!isStopped) {
        fd_set readerHandles;
        fd_set writerHandles;
        uint64_t deadlineNanoseconds = 0;

        pthread_mutex_lock(&lock);
        int fdMax = prepareHandlesLocked(&readerHandles, &writerHandles, deadlineNanoseconds);
        pthread_mutex_unlock(&lock);

        struct timeval timeout = BxSelectTimeout(deadlineNanoseconds);
        int count = select(fdMax + 1, &readerHandles, &writerHandles, NULL, &timeout);
        std::vector<BxTask*> tasks;
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno != EBADF) {
                abort();
            }

            pthread_mutex_lock(&lock);
            takeInvalidLocked(tasks);
            pthread_mutex_unlock(&lock);
            BxSelectScheduleTasks(tasks);
            continue;
        }
        if (count == 0) {
            FD_ZERO(&readerHandles);
            FD_ZERO(&writerHandles);
        }

        if (FD_ISSET(trigger.fd, &readerHandles)) {
            trigger.flush();
        }

        pthread_mutex_lock(&lock);
        takeReadyLocked(&readerHandles, &writerHandles, brixMonotonicTimeNanoseconds(), tasks);
        pthread_mutex_unlock(&lock);
        BxSelectScheduleTasks(tasks);
    }
}
