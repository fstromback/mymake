#include "std.h"
#include "process.h"

/**
 * Global process-synchronization variables.
 */

// Global process limit.
static nat procLimit = 1;

// Global active processes.
static ProcMap alive;

// Global lock for 'alive'.
static Lock aliveLock;

// Global lock for waiting on processes.
static Lock waitLock;

// System-specific waiting logic. Returns the process id that terminated.
static void systemWaitProc(ProcId &proc, int &result);

// Wait for any process we're monitoring to terminate, and handle that event.
// Assumes that 'waitLock' is taken when called.
void waitProc() {
	ProcId exited;
	int result;
	systemWaitProc(exited, result);

	Lock::Guard z(aliveLock);

	ProcMap::const_iterator i = alive.find(exited);

	if (i == alive.end())
		return;

	Process *p = i->second;
	alive.erase(i);
	if (!p)
		return;

	p->result = result;
	p->finished = true;

	if (p->owner)
		p->owner->terminated(exited, result);
}



Process::Process(const Path &file, const vector<String> &args, const Path &cwd, const Env *env) :
	file(file), args(args), cwd(cwd), env(env), process(invalidProc), owner(null), result(0), finished(false) {}

int Process::wait() {
	while (!finished) {
		Lock::Guard z(waitLock);

		// Double-check the condition after we manage to take the lock!
		if (finished)
			break;

		waitProc();
	}

	process = invalidProc;
	return result;
}


#ifdef WINDOWS

const ProcId invalidProc = INVALID_HANDLE_VALUE;

bool Process::spawn() {
	ostringstream cmdline;
	cmdline << file;
	for (nat i = 0; i < args.size(); i++)
		cmdline << ' ' << args[i];

	DEBUG("Running " << cmdline.str(), VERBOSE);

	STARTUPINFO si;
	zeroMem(si);

	si.cb = sizeof(STARTUPINFO);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi;
	zeroMem(pi);

	BOOL ok = CreateProcess(toS(file).c_str(),
							(char *)cmdline.str().c_str(),
							NULL,
							NULL,
							TRUE,
							CREATE_SUSPENDED,
							env ? env->data() : NULL,
							toS(cwd).c_str(),
							&si,
							&pi);

	if (!ok) {
		WARNING("Failed to launch " << file);
		process = invalidProc;
		return false;
	}

	process = pi.hProcess;

	{
		Lock::Guard z(aliveLock);
		alive.insert(make_pair(process, this));
	}

	// Start the process!
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);

	return true;
}

Process::~Process() {
	if (process != invalidProc) {
		{
			Lock::Guard z(aliveLock);
			alive.erase(process);
		}
		CloseHandle(process);
	}
}

static String getEnv(const char *name) {
	char buf[512] = { 0 };
	GetEnvironmentVariable(name, buf, 512);
	return buf;
}

Process *shellProcess(const String &command, const Path &cwd, const Env *env) {
	String shell = getEnv("ComSpec");

	vector<String> args;
	args.push_back("/S");
	args.push_back("/C");
	args.push_back("\"" + command + "\"");
	return new Process(Path(shell), args, cwd, env);
}

static void systemWaitProc(ProcId &proc, int &code) {
	nat size = 0;
	ProcId *ids = null;

	{
		Lock::Guard z(aliveLock);

		ids = new ProcId[alive.size()];
		for (ProcMap::const_iterator i = alive.begin(), end = alive.end(); i != end; ++i) {
			if (!i->second->terminated())
				ids[size++] = i->first;
		}
	}

	DWORD result = WaitForMultipleObjects(size, ids, FALSE, INFINITE);

	nat id = 0;
	if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + size) {
		id = result - WAIT_OBJECT_0;
	} else if (result >= WAIT_ABANDONED_0 && result < WAIT_ABANDONED_0 + size) {
		id = result - WAIT_ABANDONED_0;
	} else {
		WARNING("Failed to call WaitForMultipleObjects!");
		exit(10);
	}

	proc = ids[id];
	delete []ids;

	DWORD c = 1;
	GetExitCodeProcess(proc, &c);
	code = int(c);
}

#else

#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

const ProcId invalidProc = 0;

static char *createStr(const String &str) {
	char *n = new char[str.size() + 1];
	memcpy(n, str.c_str(), str.size() + 1);
	return n;
}

bool Process::spawn() {
	nat argc = args.size() + 1;
	char **argv = new char *[argc + 1];

	argv[0] = createStr(toS(file));

	for (nat i = 0; i < args.size(); i++) {
		argv[i + 1] = createStr(args[i]);
	}

	argv[argc] = null;

	pid_t child = fork();
	if (child == 0) {
		if (chdir(toS(cwd).c_str())) {
			PLN("Failed to chdir to " << cwd);
			exit(1);
		}

		// Wait until our parent is ready...
		raise(SIGSTOP);

		if (env) {
			execve(argv[0], argv, (char **)env->data());
		} else {
			execv(argv[0], argv);
		}
		PLN("Failed to exec!");
		exit(1);
	}

	process = child;

	{
		Lock::Guard z(aliveLock);
		alive.insert(make_pair(process, this));
	}

	// Start child again.
	kill(child, SIGCONT);

	// Clean up...
	for (nat i = 0; i < argc; i++) {
		delete []argv[i];
	}
	delete []argv;

	return true;
}

Process::~Process() {
	// We can not clean up a child without doing 'wait'. We let the system clear this when mymake
	// exits.
	if (process != invalidProc) {
		Lock::Guard z(aliveLock);
		alive.erase(process);
	}
}

Process *shellProcess(const String &command, const Path &cwd, const Env *env) {
	vector<String> args;
	args.push_back("-c");
	args.push_back(command);

	return new Process(Path("/bin/sh"), args, cwd, env);
}

static void systemWaitProc(ProcId &proc, int &result) {
	int status;
	do {
		proc = waitpid(-1, &status, 0);
	} while (!WIFEXITED(status));

	result = WEXITSTATUS(status);
}

#endif


ProcGroup::ProcGroup(nat limit) : limit(limit), failed(false) {
	if (limit == 0)
		limit = 1;
}

ProcGroup::~ProcGroup() {
	Lock::Guard z(aliveLock);

	for (ProcMap::iterator i = our.begin(), end = our.end(); i != end; ++i) {
		alive.erase(i->first);
		delete i->second;
	}
}

void ProcGroup::setLimit(nat l) {
	if (l >= 1)
		procLimit = l;
}

bool ProcGroup::canSpawn() {
	Lock::Guard z(aliveLock);
	return alive.size() < procLimit && our.size() < limit;
}

bool ProcGroup::spawn(Process *p) {
	if (failed) {
		delete p;
		return false;
	}

	p->owner = this;

	// Wait until there is room for more...
	while (!canSpawn() && !failed) {
		Lock::Guard z(waitLock);

		// Double check...
		if (canSpawn() || failed)
			break;

		waitProc();
	}

	if (failed) {
		delete p;
		return false;
	}

	p->spawn();
	our.insert(make_pair(p->process, p));

	// Make the output look more logical to the user when running on one thread.
	if (limit == 1 || procLimit == 1) {
		return wait();
	}

	return true;
}

bool ProcGroup::wait() {
	while (!failed) {
		{
			Lock::Guard z(aliveLock);
			if (our.empty())
				break;
		}

		Lock::Guard z(waitLock);

		if (failed)
			break;

		{
			Lock::Guard z(aliveLock);
			if (our.empty())
				break;
		}

		waitProc();
	}

	return !failed;
}

void ProcGroup::terminated(ProcId id, int result) {
	ProcMap::iterator i = our.find(id);
	if (i == our.end())
		return;

	if (result != 0)
		failed = true;

	delete i->second;
	our.erase(i);
}



int exec(const Path &binary, const vector<String> &args, const Path &cwd, const Env *env) {
	Process p(binary, args, cwd, env);
	if (!p.spawn())
		return 1;
	return p.wait();
}

// Run a command through a shell.
int shellExec(const String &command, const Path &cwd, const Env *env) {
	Process *p = shellProcess(command, cwd, env);
	int r = 1;
	if (p->spawn())
		r = p->wait();
	delete p;
	return r;
}
