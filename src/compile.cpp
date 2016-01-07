#include "std.h"
#include "compile.h"

bool FileQueue::empty() const {
	return q.empty();
}

bool FileQueue::any() const {
	return !q.empty();
}

void FileQueue::push(const Path &path) {
	if (s.count(path) == 0) {
		s.insert(path);
		q.push(path);
	}
}

Path FileQueue::pop() {
	Path r = q.front();
	q.pop();
	return r;
}

Compilation::Compilation(const Path &wd, const Config &config) : dir(wd), config(config) {}

bool Compilation::compile() {
	DEBUG("Compiling project in " << dir, INFO);

	FileQueue q;

	// TODO: Check pre-compiled header.
	// addFile(q, config.getStr("precompiledHeader"));

	// Add initial files.
	addFiles(q, config.getArray("input"));

	// Process files...
	while (q.any()) {
		Path now = q.pop();

		// Do we need to re-compile this file?
	}

	return false;
}

void Compilation::addFiles(FileQueue &to, const vector<String> &src) {
	for (nat i = 0; i < src.size(); i++) {
		addFile(to, src[i]);
	}
}

void Compilation::addFile(FileQueue &to, const String &src) {
	Path path(src);
	path = path.makeAbsolute(dir);

	if (!path.exists()) {
		PLN("WARNING: The file " << src << " does not exist.");
		return;
	}

	// Is the file 'x' inside the directory?

	to.push(path);
}
