#pragma once
#include "env.h"

#ifndef WINDOWS
#include <unistd.h>
#endif

// Platform specific process handle.
#ifdef WINDOWS
typedef HANDLE ProcId;
#else
typedef pid_t ProcId;
#endif

extern const ProcId invalidProc;

class ProcGroup;

/**
 * Handles a process.
 */
class Process : NoCopy {
	friend class ProcGroup;
public:
	// Create and spawn the process.
	Process(const Path &file, const vector<String> &args, const Path &cwd, const Env *env);

	// Release any resources associated with this process.
	~Process();

	// Start the process!
	bool spawn();

	// Wait until the process has terminated, and get its exit code.
	int wait();

	// Terminated?
	inline bool terminated() const { return finished; }

private:
	// Parameters.
	Path file;
	vector<String> args;
	Path cwd;
	const Env *env;

	// Handle to the process.
	ProcId process;

	// Owning group (may be null).
	ProcGroup *owner;

	// Result.
	int result;

	// Finished?
	bool finished;


	friend void waitProc();
};


typedef map<ProcId, Process *> ProcMap;


/**
 * Process group. Globally limits the number of live processes active through this class. Once one
 * process terminates with an error, the entire group will be put in a failure state.
 * TODO: Synchronize.
 */
class ProcGroup : NoCopy {
public:
	// Create.
	ProcGroup();

	// Destroy.
	~ProcGroup();

	// Set the global limit.
	static void setLimit(nat limit);

	// Spawn a new process when possible. Waits until we're below the global maximum number of
	// processes before spawning a new process. Returns false if any previous process exited with
	// an error.
	bool spawn(Process *proc);

	// Wait until all processes in this group has terminated. Returns 'false' if any of them exited
	// with an error.
	bool wait();

private:
	// Any failures so far?
	bool failed;

	// Our processes.
	ProcMap our;

	// One of our processes has terminated!
	void terminated(ProcId id, int result);


	friend void waitProc();
};

// Convenience functions.

// Create a Process executing things in a shell.
Process *shellProcess(const String &command, const Path &cwd, const Env *env);

// Low-level exec.
int exec(const Path &binary, const vector<String> &args, const Path &cwd, const Env *env);

// Run a command through a shell.
int shellExec(const String &command, const Path &cwd, const Env *env);
