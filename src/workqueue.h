#pragma once
#include "sync.h"

/**
 * A queue suitable for using as a list of work for multiple consumers. "pop" blocks if there are
 * currently no elements in the queue.
 *
 * Also includes the concept of "we're done". When a thread calls 'end', the thread will be emptied,
 * and then all future calls to "pop" will return null without blocking.
 */
template <class T>
class WorkQueue : NoCopy {
public:
	WorkQueue() : sema(0) {}

	// Push an element.
	void push(T *elem) {
		Lock::Guard z(lock);
		q.push(elem);
		sema.up();
	}

	// Pop an element. Returns null on failure.
	T *pop() {
		sema.down();
		Lock::Guard z(lock);

		if (q.empty()) {
			// If the queue is empty at this point, we know that someone called "done". Thus, we
			// need to up the semaphore again so that all future calls to pop() will return.
			sema.up();
			return null;
		} else {
			T *elem = q.front();
			q.pop();
			return elem;
		}
	}

	// Inform any threads waiting that we are done. Designed to only be called once.
	void done() {
		// We implement the idea of being done by simply making the semaphore out of sync with the
		// elements in the queue.
		sema.up();
	}

private:
	queue<T *> q;

	// Lock for the queue.
	Lock lock;

	// Semaphore representing number of elements in the queue.
	Sema sema;
};
