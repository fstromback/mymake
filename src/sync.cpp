#include "std.h"
#include "sync.h"

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

Sema::Sema(nat init) {
	sema = CreateSemaphore(NULL, init, 100, NULL);
}

Sema::~Sema() {
	CloseHandle(sema);
}

void Sema::up() {
	ReleaseSemaphore(sema, 1, NULL);
}

void Sema::down() {
	WaitForSingleObject(sema, INFINITE);
}

#else

Lock::Lock() {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);
	pthread_mutexattr_destroy(&attr);
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

Sema::Sema(nat init) {
	sem_init(&sema, 0, init);
}

Sema::~Sema() {
	sem_destroy(&sema);
}

void Sema::up() {
	sem_post(&sema);
}

void Sema::down() {
	sem_wait(&sema);
}

#endif


Condition::Condition() : sema(0) {}

void Condition::wait() {
	sema.down();
	// It was signaled, leave it signaling!
	sema.up();
}

void Condition::signal() {
	sema.up();
}
