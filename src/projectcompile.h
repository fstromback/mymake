#pragma once
#include "compile.h"
#include "pathqueue.h"
#include "toposort.h"

namespace compile {

	/**
	 * Data for compiling an entire project.
	 */
	class Project : NoCopy {
	public:
		// Create.
		Project(const Path &wd, const set<String> &cmdlineOptions, const MakeConfig &projectFile, const Config &config);

		// Destroy.
		~Project();

		// Find dependencies.
		bool find();

		// Compile.
		bool compile();

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
	};

}
