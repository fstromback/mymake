#pragma once
#include "path.h"
#include "hash.h"

/**
 * Cache for file extensions.
 *
 * Allows quick queries of the type: Are there any files with a given name and a specified
 * extension. If so, which?
 *
 * The approach taken here is to list files in a directory and preemptively find the answers to all
 * possible (valid) queries ahead of time, rather than looking for the answer by trying all
 * combinations each time.
 */
class ExtCache {
public:
	// Create. Specify which extensions are interesting.
	ExtCache(const vector<String> &exts);

	// Get valid extensions for a file.
	const vector<String> &find(Path path);

private:
	struct PathCompare {
		bool operator() (const String &a, const String &b) const {
			return Path::compare(a, b) < 0;
		}
	};

	// Valid extensions.
	set<String, PathCompare> validExts;

	// Set of the paths we have explored.
	hash_set<Path> explored;

	// Map of paths (file titles) to the available extensions.
	typedef hash_map<Path, vector<String>> ExtMap;
	ExtMap exts;

	// Empty vector that we return in case of a cache miss.
	vector<String> empty;

	// Explore a path.
	void explorePath(const Path &path);
};
