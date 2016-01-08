#pragma once
#include "path.h"

/**
 * Ensures no items are inserted twice.
 */

template <class T>
class UniqueQueue {
public:
	// Empty?
	bool empty() const {
		return q.empty();
	}

	bool any() const {
		return !empty();
	}

	// Pop top element.
	T pop() {
		T r = q.front();
		q.pop();
		return r;
	}

	// Push element, discards it if it has ever been in the queue before.
	void push(const T &elem) {
		if (s.count(elem) == 0) {
			s.insert(elem);
			q.push(elem);
		}
	}

	UniqueQueue<T> &operator <<(const T &elem) {
		push(elem);
		return *this;
	}

private:
	// Files in the queue.
	set<T> s;

	// Queue of files.
	queue<T> q;
};


typedef UniqueQueue<Path> PathQueue;
