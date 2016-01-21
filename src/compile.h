#pragma once
#include "path.h"
#include "config.h"
#include "pathqueue.h"
#include "includes.h"
#include "wildcard.h"
#include "process.h"
#include "env.h"

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

		// Clean this target.
		void clean();

		// Find all files in the working directory.
		bool find();

		// Compile a directory with a .mymake file in.
		bool compile(const String &prefix = "");

		// Execute the final executable.
		int execute(const vector<String> &params) const;

		// Depends on these projects:
		set<String> dependsOn;

		// Output file.
		Path output;

		// Link the output file to any projects that depends on this one?
		bool linkOutput;

		// Link our dependencies to targets dependent on us.
		bool forwardDeps;

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

		// Pch header.
		String pchHeader;

		// Pch file.
		Path pchFile;

		// Combined pch generation+compilation.
		bool combinedPch;

		typedef UniqueQueue<Compile> CompileQueue;

		// Files to compile in some valid order.
		vector<Compile> toCompile;

		// Run steps.
		bool runSteps(const String &key, ProcGroup &group, const Env &env, const map<String, String> &options);

		// Add files to the queue of files to process.
		void addFiles(CompileQueue &to, const vector<String> &src);

		// Add a file to the queue of files to process.
		void addFile(CompileQueue &to, const String &src, bool pch);

		// Add a file to the queue of files to process.
		void addFile(CompileQueue &to, const Path &src);

		// Cache of the directory contents for 'findExt'.

		// Map of all file titles found along with their extension. Paths here are relative.
		typedef map<Path, vector<String>> CacheItem;

		// Map of all files in searched paths.
		typedef map<Path, CacheItem> CacheMap;
		CacheMap findExtCache;

		// Build the cache for a specific directory.
		CacheItem buildCache(const Path &dir) const;

		// Find a valid extension for the given Path.
		bool findExt(Path &to);

		// Choose a file to compile.
		String chooseCompile(const String &file);

		// Find files recursively.
		void addFilesRecursive(CompileQueue &to, const Path &root);

		// File ignored?
		bool ignored(const String &file);

	};

}
