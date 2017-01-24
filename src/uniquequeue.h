#pragma once
#include "path.h"
#include "hash.h"

/**
 * Ensures no items are inserted twice.
 *
 * If 'K' is set, values are compared for equality using type K instead of type V.
 */

template <class V, class K = V>
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
	V pop() {
		V r = q.front();
		q.pop();
		return r;
	}

	// Push element, discards it if it has ever been in the queue before.
	void push(const V &elem) {
		if (s.count(elem) == 0) {
			s.insert(elem);
			q.push(elem);
		}
	}

	UniqueQueue<V, K> &operator <<(const V &elem) {
		push(elem);
		return *this;
	}

private:
	// Files in the queue.
	hash_set<K> s;

	// Queue of files.
	queue<V> q;
};


typedef UniqueQueue<Path> PathQueue;
