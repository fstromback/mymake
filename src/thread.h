#pragma once
#include "sync.h"

/**
 * A platform-independent thread.
 */
class Thread : NoCopy {
public:
	// Create the thread object, but does not start the thread.
	Thread();

	// Destroy the thread object. The thread keeps running.
	~Thread();

	// Start the thread.
	void start(void (*fn)());

	template <class T>
	void start(void (*fn)(T &), T &data);

	template <class T>
	void start(void (T::*fn)(), T &data);

	// Wait for the thread to terminate.
	void join();

	// Adapter struct for data to rawStart.
	class Data : NoCopy {
	public:
		// Called on startup.
		virtual void start() = 0;
	};

private:
	// Raw thread handle.
#ifdef WINDOWS
	HANDLE thread;
#else
	pthread_t thread;
#endif

	// Started?
	bool started;

	// C-style start.
	void rawStart(Data *data);
};


// Implementation of template members.
template <class T>
void Thread::start(void (*fn)(T &), T &data) {
	class D : public Data {
	public:
		void (*fn)(T &);
		T *data;

		virtual void start() {
			(*fn)(data);
		}

		D(void (*fn)(T &), T *data) : fn(fn), data(data) {}
	};

	rawStart(new D(fn, &data));
}

template <class T>
void Thread::start(void (T::*fn)(), T &data) {
	class D : public Data {
	public:
		void (T::*fn)();
		T *data;

		virtual void start() {
			(data->*fn)();
		}

		D(void (T::*fn)(), T *data) : fn(fn), data(data) {}
	};

	rawStart(new D(fn, &data));
}
