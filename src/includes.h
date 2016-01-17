#pragma once
#include "path.h"
#include "pathqueue.h"
#include "config.h"
#include "wildcard.h"

/**
 * Error with includes.
 */
class IncludeError : public Error {
public:
	IncludeError(const String &msg) : msg(msg) {}
	inline ~IncludeError() throw () {}
	const char *what() const throw() { return msg.c_str(); }
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
	IncludeInfo(const Path &file, bool ignored = false);

	// The file itself.
	Path file;

	// The first included file (if any).
	String firstInclude;

	// All files included from this file.
	set<Path> includes;

	// Is this file ignored?
	bool ignored;

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
	Path resolveInclude(const Path &first, const Path &fromFile, nat lineNr, const String &inc) const;

	// Load the cache from file.
	void load(const Path &from);

	// Save the cache to file.
	void save(const Path &to) const;

	// Set patterns for ignored files.
	void ignore(const vector<String> &patterns);

private:
	// Ignored patterns.
	vector<Wildcard> ignorePatterns;

	// Check if a path is ignored.
	bool ignored(const Path &path) const;

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
	IncludeInfo recursiveIncludesIn(const Path &file);

	// Find includes in file.
	void includesIn(const Path &firstFile, const Path &file, PathQueue &to, String *firstInclude);

};
