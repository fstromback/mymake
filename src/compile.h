#pragma once
#include "path.h"
#include "config.h"
#include "pathqueue.h"
#include "includes.h"
#include "wildcard.h"

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

		// Depends on these projects:
		set<String> dependsOn;

		// Output file.
		Path output;

		// Link the output file to any projects that depends on this one?
		bool linkOutput;

		// Add a library.
		void addLib(const Path &path);

	private:
		// Working directory.
		Path wd;

		// Configuration.
		Config config;

		// Include cache.
		Includes includes;

		// Valid extensions to compile.
		vector<String> validExts;

		// Compilation command lines.
		vector<String> compileVariants;

		// Build directory.
		Path buildDir;

		// Files to ignore.
		vector<Wildcard> ignore;

		// Output file type.
		String intermediateExt;

		// File info.
		class Compile : public Path {
		public:
			// Precompiled header?
			bool isPch;

			// From automatic search.
			bool autoFound;

			inline Compile(const Path &file, bool pch, bool a) : Path(file), isPch(pch), autoFound(a) {}
		};

		// Pch file.
		Path pchFile;

		typedef UniqueQueue<Compile> CompileQueue;

		// Files to compile in some valid order.
		vector<Compile> toCompile;

		// Run steps.
		bool runSteps(const String &key);

		// Add files to the queue of files to process.
		void addFiles(CompileQueue &to, const vector<String> &src);

		// Add a file to the queue of files to process.
		void addFile(CompileQueue &to, const String &src, bool pch);

		// Add a file to the queue of files to process.
		void addFile(CompileQueue &to, const Path &src);

		// Find a valid extension for the given Path.
		bool findExt(Path &to) const;

		// Choose a file to compile.
		String chooseCompile(const String &file);

		// Find files recursively.
		void addFilesRecursive(CompileQueue &to, const Path &root);

	};

}
