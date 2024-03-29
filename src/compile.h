#pragma once
#include "path.h"
#include "config.h"
#include "uniquequeue.h"
#include "includes.h"
#include "commands.h"
#include "extcache.h"
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
		bool compile();

		// Save build files (include cache, etc.)
		void save() const;

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

		// Previous command lines.
		Commands commands;

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

		// Append extension to the names of intermediate files.
		bool appendExt;

		// Absolute paths to the compiler?
		bool absolutePath;

		// Transform the path to absolute/relative as set up by the config.
		String preparePath(const Path &path);

		typedef UniqueQueue<Compile, Path> CompileQueue;

		// Files to compile in some valid order.
		vector<Compile> toCompile;

		// Create a shellProcess instance that saves the output to 'commands' whenever the command succeeds.
		Process *saveShellProcess(const String &file, const String &command, const Path &cwd, nat skip);

		// Run steps.
		bool runSteps(const String &key, ProcGroup &group, const map<String, String> &options);

		// Add files to the queue of files to process.
		void addFiles(CompileQueue &to, ExtCache &cache, const vector<String> &src);

		// Add a file to the queue of files to process.
		bool addFile(CompileQueue &to, ExtCache &cache, const String &src, bool pch);

		// Add a file to the queue of files to process.
		void addFile(CompileQueue &to, ExtCache &cache, const Path &src);

		// Add files created by the pre-build steps. These might not exist yet.
		void addPreBuildFiles(CompileQueue &to, const vector<String> &src);
		void addPreBuildFile(CompileQueue &to, const String &src);

		// Find a valid extension for the given Path.
		vector<Path> findExt(const Path &to, ExtCache &cache);

		// Choose a file to compile.
		String chooseCompile(const String &file);

		// Find files recursively.
		void addFilesRecursive(CompileQueue &to, const Path &root);

		// File ignored?
		bool ignored(const String &file);

	};

}
