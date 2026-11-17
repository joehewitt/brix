
#ifndef BX_MUTEX_H
#define BX_MUTEX_H

#include <BxEvents/BxEvents.h>

#include <pthread.h>

// *************************************************************************************************

class BxMutex {
public:
    BxMutex();
    ~BxMutex();

    bool tryLock();
    void lock();
    void unlock();

private:
    pthread_mutex_t mutex;
};

// *************************************************************************************************

#endif
