#include "std.h"
#include "thread.h"

#ifdef WINDOWS

Thread::Thread() : started(false) {}

Thread::~Thread() {
	if (started) {
		CloseHandle(thread);
	}
}

static DWORD WINAPI threadMain(void *d) {
	Thread::Data *data = (Thread::Data *)d;
	data->start();
	delete data;

	return 0;
}

void Thread::rawStart(Data *data) {
	if (started)
		return;

	started = true;
	thread = CreateThread(NULL, 0, &threadMain, data, 0, NULL);
}

void Thread::join() {
	if (!started)
		return;

	started = false;
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
}

#else

Thread::Thread() : started(false) {}

Thread::~Thread() {
	if (started) {
		pthread_detach(thread);
	}
}

static void *threadMain(void *d) {
	Thread::Data *data = (Thread::Data *)d;
	data->start();
	delete data;

	return null;
}

void Thread::rawStart(Data *data) {
	if (started)
		return;

	started = true;
	pthread_create(&thread, null, &threadMain, data);
}

void Thread::join() {
	if (!started)
		return;

	started = false;
	pthread_join(thread, null);
}

#endif


void Thread::start(void (*fn)()) {
	class D : public Data {
	public:
		void (*fn)();

		virtual void start() {
			(*fn)();
		}

		D(void (*fn)()) : fn(fn) {}
	};

	rawStart(new D(fn));
}
