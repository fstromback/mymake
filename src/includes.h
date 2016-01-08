#pragma once
#include "path.h"
#include "pathqueue.h"
#include "config.h"

/**
 * Error with includes.
 */
class IncludeError : public Error {
public:
	IncludeError(const String &msg) : msg(msg) {}
	const char *what() const { return msg.c_str(); }
private:
	String msg;
};

/**
 * Information about a single file.
 */
class IncludeInfo {
public:
	// Create.
	IncludeInfo();
	IncludeInfo(const Path &file);

	// The file itself.
	Path file;

	// All files included from this file.
	set<Path> includes;

	// Compute the last modified date of all includes.
	Timestamp lastModified() const;
};

// Output.
ostream &operator <<(ostream &to, const IncludeInfo &i);

/**
 * Keep a cache of all includes from specific files.
 */
class Includes {
public:
	// Give information on include paths.
	Includes(const vector<Path> &includePaths);
	Includes(const Path &wd, const Config &config);

	// Get includes, and latest modified time from one include.
	IncludeInfo info(const Path &file);

	// Resolve an include string given the include path(s).
	Path resolveInclude(const Path &fromFile, const String &inc) const;

	// Load the cache from file.
	void load(const Path &from);

	// Save the cache to file.
	void save(const Path &to) const;

private:
	// Include search paths. The root is always first.
	vector<Path> includePaths;

	// Information about a single file (+ time added to cache).
	struct Info {
		Info();
		Info(const IncludeInfo &info, const Timestamp &time);
		Info(const Path &file, const Timestamp &time);

		// Regular info.
		IncludeInfo info;

		// Last modified as seen by the cache.
		Timestamp lastModified;

		// Up to date?
		bool upToDate() const;
	};

	// Information about each file.
	typedef map<Path, Info> InfoMap;
	InfoMap cache;

	// Find includes in file, and all files included from there.
	set<Path> recursiveIncludesIn(const Path &file);

	// Find includes in file.
	void includesIn(const Path &file, PathQueue &to);

};
