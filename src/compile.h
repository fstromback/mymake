#pragma once
#include "path.h"
#include "config.h"
#include "pathqueue.h"
#include "includes.h"

namespace compile {

    /**
     * Mymake compilation.
     */
	class Target {
	public:
		// 'wd' is the directory with the .mymake file in it.
		Target(const Path &wd, const Config &config);

		// Compile a directory with a .mymake file in.
		bool compile();

	private:
		// Working directory.
		Path dir;

		// Configuration.
		Config config;

		// Include cache.
		Includes includes;

		// Add files to the queue of files to process.
		void addFiles(PathQueue &to, const vector<String> &src);

		// Add a file to the queue of files to process.
		void addFile(PathQueue &to, const String &src);

	};

}
