#include "std.h"
#include "extcache.h"

ExtCache::ExtCache(const vector<String> &exts) : validExts(exts.begin(), exts.end()) {}

const vector<String> &ExtCache::find(Path path) {
	Path parent = path.parent();
	if (explored.count(parent) == 0)
		explorePath(parent);

	path.makeExt("");
	ExtMap::const_iterator i = exts.find(path);
	if (i == exts.end())
		return empty;
	else
		return i->second;
}

void ExtCache::explorePath(const Path &path) {
	explored.insert(path);

	vector<Path> children = path.children();
	for (nat i = 0; i < children.size(); i++) {
		Path &p = children[i];

		if (p.isDir())
			continue;

		String ext = p.ext();
		if (validExts.count(ext) == 0)
			continue;

		p.makeExt("");
		exts[p].push_back(ext);
	}
}
