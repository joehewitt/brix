
#include <YuEvents/YuMutex.h>

// *************************************************************************************************


YuMutex::YuMutex() {
    pthread_mutex_init(&mutex, 0);
}

YuMutex::~YuMutex() {
    pthread_mutex_destroy(&mutex);
}

bool
YuMutex::tryLock() {
    return pthread_mutex_trylock(&mutex) == 0;
}

void
YuMutex::lock() {
    pthread_mutex_lock(&mutex);
}

void
YuMutex::unlock() {
    pthread_mutex_unlock(&mutex);
}
