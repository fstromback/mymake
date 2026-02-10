#pragma once
#include "compile.h"
#include "uniquequeue.h"
#include "toposort.h"
#include "sync.h"
#include "thread.h"
#include "workqueue.h"

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

		// Number of threads to use.
		nat numThreads;

		// Use prefix when building in parallel?
		String usePrefix;

		// Information about a target and all it dependencies.
		typedef Node<String> TargetDeps;

		// Found targets, in the order we found them. Compiling in this order ensures all
		// dependencies are fullfilled. This also determines the order in which libraries are linked
		// (this is important on GCC for example). In this case, we use the reverse order due to how
		// the linker works.
		vector<TargetDeps> order;

		// The target we want to run.
		String mainTarget;

		// Information associated with a target.
		class TargetInfo : NoCopy {
		public:
			TargetInfo(const String &name, bool fromImplicitDep)
				: name(name), fromImplicitDep(fromImplicitDep), order(0), status(sNotReady) {}

			~TargetInfo() {
				delete target;
			}

			// Name of this target.
			String name;

			// Was this info created as a result of an implicit dependency? (Ignores some warnings,
			// don't use to trigger options, etc.)
			bool fromImplicitDep;

			// The target itself, if loaded.
			Target *target;

			// Dependencies of the target.
			set<String> depends;

			// Index in the computed compilation order (for convenient reverse lookups).
			nat order;

			// Status:
			enum Status {
				sNotReady,
				sOK,
				sError,
			};

			// Type is of 'status', used as a nat since updated concurrently.
			volatile nat status;

			// Semaphore for synch. Signalled when status becomes ready.
			Condition done;

			// Create a "TargetDeps" element.
			TargetDeps node() {
				TargetDeps t = { name, depends };
				return t;
			}
		};

		class TargetSort;

		// Targets.
		map<String, TargetInfo *> target;

		// Shared state for multithreaded find. Contains two queues: one queue that is used by the
		// main, coordinating, thread, and another that is used for distributing work between the
		// worker threads. If 'numThreads' in the Project object is 1, then we don't spawn any
		// threads.
		class FindState : NoCopy {
		public:
			FindState(Project *p);

			~FindState();

			// Using multiple threads?
			bool isMT() const { return threads != null; }

			// Push an element.
			void push(TargetInfo *info);

			// Pop an element from the "process" thread. This ensures that the element has been
			// processed. Elements are guaranteed to be returned in the same order they were
			// pushed. Returns null if empty.
			TargetInfo *pop();

			// Call when we are done.
			void done();

		private:
			// Owner.
			Project *project;

			// Threads (if created).
			Thread *threads;

			// Work queue.
			WorkQueue<TargetInfo> work;

			// Processing queue.
			queue<TargetInfo *> process;

			// Entry point for threads.
			void main();
		};

		// Add targets to an UniqueQueue.
		void addTargets(const vector<String> &names, FindState &to);
		void addTarget(const String &name, bool fromImplicitDep, FindState &to);

		// Prepare a target - i.e. does all processing (loading, looking up dependencies) that can
		// be done in parallel.
		void prepareTarget(TargetInfo *info) const;

		// Create a new target.
		Target *loadTarget(const String &name) const;

		// Propagate dependencies between targets. This is done right after we find the dependencies
		// between all targets, while we are still running in a single thread. We also assume that
		// the function is called in reverse compilation order (i.e. if X is visited first, we know
		// that all dependencies of X have not yet been called). That way we avoid duplicates.
		void propagateDependencies(const String &name);

		// Information about dependencies from some target.
		struct LibDeps {
			// Local library itself.
			vector<Path> local;

			// Other libraries.
			vector<String> external;
		};

		// Find dependencies to a target. Duplicates are removed. Returns order-id -> output file name.
		map<nat, LibDeps> dependencies(const TargetInfo *info) const;
		void dependencies(const String &root, vector<bool> &visited, map<nat, LibDeps> &out, const TargetInfo *at) const;

		// Compile one target. This function may only _read_ from shared data.
		bool compileOne(nat id, bool mt);

		// Compile single-threaded.
		bool compileST();

		// Shared state for the multithreaded compilation.
		class MTCompileState : NoCopy {
		public:
			// Owning project.
			Project *p;

			// Condition variables signaled when each target has been compiled.
			map<String, Condition *> targetDone;

			// Next target to process.
			nat next;

			// Compilation ok? (= no errors).
			volatile nat ok;

			// Create.
			MTCompileState(Project &p);

			// Destroy.
			~MTCompileState();

			// Launch.
			void start();
		};

		// Compile multi-threaded (using N threads).
		bool compileMT(nat threads);

		// Main function for each thread in compileMT.
		bool threadMain(MTCompileState &state);

	};

}
