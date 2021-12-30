#pragma once
#include "path.h"
#include "hash.h"
#include "config.h"
#include "wildcard.h"
#include "timeCache.h"

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
	typedef set<Path> PathSet;
	PathSet includes;

	// Is this file ignored? (ie. not useful to look for headers inside?)
	bool ignored;

	// Compute the last modified date of all includes.
	Timestamp lastModified(TimeCache &cache) const;
};

// Output.
ostream &operator <<(ostream &to, const IncludeInfo &i);

/**
 * Keep a cache of all includes from specific files.
 */
class Includes {
public:
	// Give information on include paths.
	Includes(const Path &wd, const vector<Path> &includePaths);
	Includes(const Path &wd, const Config &config);

	// Get includes, and latest modified time from one include.
	const IncludeInfo &info(const Path &file);

	// Resolve an include string given the include path(s).
	Path resolveInclude(const Path &file, nat lineNr, const String &inc) const;

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

	// Current working directory.
	Path wd;

	// Include search paths. The root is always first.
	vector<Path> includePaths;

	// Internal representation of a single file, both headers and cpp-files are stored this way.
	// This makes it possible to only look for includes in a header once, and re-use that
	// information for other files.
	struct Info {
		Info();
		Info(const Path &file);

		// File name.
		Path file;

		// First file included (if any).
		String firstInclude;

		// All files included from this file.
		set<Path> includes;

		// The timestamp of the file last time we looked at it.
		Timestamp lastModified;

		// Is this file ignored? Ie, we did not even try too look for headers here?
		bool ignored;

		// Valid?
		bool valid;
	};

	// Information about each file. If a file is in the cache, it is valid (ie. it is not too
	// old). We save this cache to disk between runs of mymake.
	typedef map<Path, Info> InfoMap;
	InfoMap cache;

	// Get an Info struct for a specific entry, creating it if it does not already exist.
	const Info &fileInfo(const Path &file);

	// Create the file info to be inserted into the cache.
	Info createFileInfo(const Path &file);

	// Cache for the call to "info".
	typedef hash_map<Path, IncludeInfo> RecInfoMap;
	RecInfoMap recCache;

	// Create an includeinfo object.
	IncludeInfo createInfo(const Path &file);
};
