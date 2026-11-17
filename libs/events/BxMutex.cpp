
#include <BxEvents/BxMutex.h>

// *************************************************************************************************


BxMutex::BxMutex() {
    pthread_mutex_init(&mutex, 0);
}

BxMutex::~BxMutex() {
    pthread_mutex_destroy(&mutex);
}

bool
BxMutex::tryLock() {
    return pthread_mutex_trylock(&mutex) == 0;
}

void
BxMutex::lock() {
    pthread_mutex_lock(&mutex);
}

void
BxMutex::unlock() {
    pthread_mutex_unlock(&mutex);
}
