#pragma once
#include "path.h"
#include "hash.h"

/**
 * A simple cache for file-time queries.
 */
class TimeCache : NoCopy {
public:
	// Query the cache.
	const FileInfo &info(const Path &path);

	// Get the modified time.
	Timestamp mTime(const Path &path) {
		return info(path).mTime;
	}

private:
	typedef hash_map<Path, FileInfo> Cache;
	Cache cache;
};
