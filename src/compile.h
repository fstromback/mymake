#pragma once
#include "path.h"
#include "config.h"

/**
 * Queue of files to compile.
 */
class FileQueue {
public:
	// Empty?
	bool empty() const;
	bool any() const;

	// Pop top element.
	Path pop();

	// Push element, discards it if it has ever been in the queue before.
	void push(const Path &path);

private:
	// Files in the queue.
	set<Path> s;

	// Queue of files.
	queue<Path> q;
};

/**
 * Mymake compilation.
 */
class Compilation {
public:
	// 'wd' is the directory with the .mymake file in it.
	Compilation(const Path &wd, const Config &config);

	// Compile a directory with a .mymake file in.
	bool compile();

private:
	// Working directory.
	Path dir;

	// Configuration.
	Config config;

	// Add files to the queue of files to process.
	void addFiles(FileQueue &to, const vector<String> &src);

	// Add a file to the queue of files to process.
	void addFile(FileQueue &to, const String &src);

};
