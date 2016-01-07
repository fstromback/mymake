#include "std.h"
#include "compile.h"

namespace compile {

	Target::Target(const Path &wd, const Config &config) : dir(wd), config(config), includes(wd, config) {}

	bool Target::compile() {
		DEBUG("Compiling project in " << dir, INFO);

		PathQueue q;

		// TODO: Check pre-compiled header.
		// addFile(q, config.getStr("precompiledHeader"));

		// Add initial files.
		addFiles(q, config.getArray("input"));

		// Process files...
		while (q.any()) {
			Path now = q.pop();
			PVAR(now);

			// Do we need to re-compile this file?
			IncludeInfo info = includes.info(now);
		}

		return false;
	}

	void Target::addFiles(PathQueue &to, const vector<String> &src) {
		for (nat i = 0; i < src.size(); i++) {
			addFile(to, src[i]);
		}
	}

	void Target::addFile(PathQueue &to, const String &src) {
		Path path(src);
		path = path.makeAbsolute(dir);

		// TODO: if the file has no or an invalid extension, add one and try it.

		if (!path.exists()) {
			WARNING("The file " << src << " does not exist.");
			return;
		}

		// Is the file 'x' inside the directory?

		PVAR(path);
		to.push(path);
	}

}
