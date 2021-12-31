#include "std.h"
#include "timeCache.h"

const FileInfo &TimeCache::info(const Path &path) {
	Cache::const_iterator i = cache.find(path);
	if (i == cache.end()) {
		return cache.insert(make_pair(path, path.info())).first->second;
	} else {
		return i->second;
	}
}
