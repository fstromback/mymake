#include "std.h"
#include "lock.h"

#ifdef WINDOWS

Lock::Lock() {
	InitializeCriticalSection(&lock);
}

Lock::~Lock() {
	DeleteCriticalSection(&lock);
}

Lock::Guard::Guard(Lock &lock) : lock(lock) {
	EnterCriticalSection(&lock.lock);
}

Lock::Guard::~Guard() {
	LeaveCriticalSection(&lock.lock);
}

#else

Lock::Lock() {
	pthread_mutex_init(&lock, null);
}

Lock::~Lock() {
	pthread_mutex_destroy(&lock);
}

Lock::Guard::Guard(Lock &lock) : lock(lock) {
	pthread_mutex_lock(&lock.lock);
}

Lock::Guard::~Guard() {
	pthread_mutex_unlock(&lock.lock);
}

#endif
