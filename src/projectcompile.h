#pragma once
#include "compile.h"
#include "uniquequeue.h"
#include "toposort.h"
#include "sync.h"

namespace compile {

	/**
	 * Data for compiling an entire project.
	 */
	class Project : NoCopy {
	public:
		// Create.
		Project(const Path &wd, const set<String> &cmdlineOptions, const MakeConfig &projectFile, const Config &config, bool showTimes);

		// Destroy.
		~Project();

		// Find dependencies.
		bool find();

		// Clean.
		void clean();

		// Compile.
		bool compile();

		// Save build files.
		void save() const;

		// Execute the resulting file.
		int execute(const vector<String> &params);

	private:
		// Working directory for the project.
		Path wd;

		// Raw configuration data = contents of project file.
		MakeConfig projectFile;

		// Configuration (global, cmdline included).
		Config config;

		// Configuration (only build-section)
		Config buildConfig;

		// Configuration (only deps-section)
		Config depsConfig;

		// Ignore sub-directories without a .mymake-file in them.
		bool explicitTargets;

		// Use implicit dependencies.
		bool implicitDependencies;

		// Show compilation times.
		bool showTimes;

		// Use prefix when building in parallel?
		String usePrefix;

		// Information about a target and all it dependencies.
		typedef Node<String> TargetInfo;

		// Found targets, in the order we found them. Compiling in reverse order ensures all dependencies
		// are fullfilled.
		vector<TargetInfo> order;

		// The target we want to run.
		String mainTarget;

		// Targets.
		map<String, Target *> target;

		// Info.
		map<String, TargetInfo> targetInfo;

		// Queue for the targets.
		typedef UniqueQueue<String> TargetQueue;

		// Add targets to an UniqueQueue.
		void addTargets(const vector<String> &names, TargetQueue &to);
		void addTarget(const String &name, TargetQueue &to);

		// Create a new target.
		Target *loadTarget(const String &name) const;

		// Find dependencies to a target. May contain duplicates.
		vector<Path> dependencies(const String &root, const TargetInfo &at) const;
		void dependencies(const String &root, vector<Path> &out, const TargetInfo &at) const;

		// Compile one target. This function may only _read_ from shared data.
		bool compileOne(nat id, bool mt);

		// Compile single-threaded.
		bool compileST();

		// Shared state for the multithreaded compilation.
		struct MTState : NoCopy {
			// Owning project.
			Project *p;

			// Condition variables signaled when each target has been compiled.
			map<String, Condition *> targetDone;

			// Next target to process.
			nat next;

			// Compilation ok? (= no errors).
			volatile bool ok;

			// Create.
			MTState(Project &p);

			// Destroy.
			~MTState();

			// Launch.
			void start();
		};

		// Compile multi-threaded (using N threads).
		bool compileMT(nat threads);

		// Main function for each thread in compileMT.
		bool threadMain(MTState &state);
	};

}
