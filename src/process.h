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

	// Wait until the process has terminated, and get its exit code.
	int wait();

	// Start the process!
	bool spawn();

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
};

/**
 * Process group. Globally limits the number of live processes active through this class.
 * It is assumed that, while using a ProcGroup, noone tries to call Process::wait.
 * TODO: Synchronize.
 */
class ProcGroup : NoCopy {
public:
	// Create.
	ProcGroup();

	// Set the global limit.
	static void setLimit(nat limit);

	// Spawn a new process when possible.
	void spawn(Process *proc);

	// Wait until we can afford to start a new process. If an earlier of our processes have been
	// terminated, return the result from that process.
	int wait();

private:
	// Global process limit.
	static nat limit;

	// Global active processes.
	static map<ProcId, Process *> alive;

	// Return codes for this process group.
	queue<int> errors;

	// Platform specific wait code.
	void waitChild(ProcId &process, int &status);
};

// Convenience functions.

// Create a Process executing things in a shell.
Process *shellProcess(const String &command, const Path &cwd, const Env *env);

// Low-level exec.
int exec(const Path &binary, const vector<String> &args, const Path &cwd, const Env *env);

// Run a command through a shell.
int shellExec(const String &command, const Path &cwd, const Env *env);
