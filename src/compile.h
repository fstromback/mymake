#pragma once
#include "path.h"
#include "config.h"
#include "pathqueue.h"
#include "includes.h"

namespace compile {

    /**
     * Mymake compilation.
     */
	class Target : NoCopy {
	public:
		// 'wd' is the directory with the .mymake file in it.
		Target(const Path &wd, const Config &config);

		// Saves some caches.
		~Target();

		// Find all files in the working directory.
		bool find();

		// Compile a directory with a .mymake file in.
		bool compile();

		// Execute the final executable.
		int execute(const vector<String> &params) const;

	private:
		// Working directory.
		Path wd;

		// Build directory.
		Path buildDir;

		// Configuration.
		Config config;

		// Include cache.
		Includes includes;

		// Valid extensions to compile.
		vector<String> validExts;

		// Compilation command lines.
		vector<String> compileVariants;

		// Output file type.
		String intermediateExt;

		// Files to compile in some valid order.
		vector<Path> toCompile;

		// Output file.
		Path output;

		// Add files to the queue of files to process.
		void addFiles(PathQueue &to, const vector<String> &src);

		// Add a file to the queue of files to process.
		void addFile(PathQueue &to, const String &src);

		// Add a file to the queue of files to process.
		void addFile(PathQueue &to, const Path &src);

		// Find a valid extension for the given Path.
		bool findExt(Path &to) const;

		// Choose a file to compile.
		String chooseCompile(const String &file);

	};

}
