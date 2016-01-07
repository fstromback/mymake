#include "std.h"
#include "pathqueue.h"

bool PathQueue::empty() const {
	return q.empty();
}

bool PathQueue::any() const {
	return !q.empty();
}

void PathQueue::push(const Path &path) {
	if (s.count(path) == 0) {
		s.insert(path);
		q.push(path);
	}
}

PathQueue &PathQueue::operator <<(const Path &path) {
	push(path);
	return *this;
}

Path PathQueue::pop() {
	Path r = q.front();
	q.pop();
	return r;
}
