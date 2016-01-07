#pragma once
#include "path.h"

/**
 * Queue of paths. Ensures no files are inserted twice.
 */
class PathQueue {
public:
	// Empty?
	bool empty() const;
	bool any() const;

	// Pop top element.
	Path pop();

	// Push element, discards it if it has ever been in the queue before.
	void push(const Path &path);
	PathQueue &operator <<(const Path &path);

private:
	// Files in the queue.
	set<Path> s;

	// Queue of files.
	queue<Path> q;
};
