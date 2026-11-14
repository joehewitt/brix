
#ifndef YU_MUTEX_H
#define YU_MUTEX_H

#include <YuEvents/YuEvents.h>

#include <pthread.h>

// *************************************************************************************************

class YuMutex {
public:
    YuMutex();
    ~YuMutex();

    bool tryLock();
    void lock();
    void unlock();

private:
    pthread_mutex_t mutex;
};

// *************************************************************************************************

#endif
