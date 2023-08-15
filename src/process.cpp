#include "std.h"
#include "process.h"
#include "outputmgr.h"

/**
 * Global process-synchronization variables.
 */

// Global process limit.
static nat procLimit = 1;

// Global active processes.
static ProcMap alive;

// Global lock for 'alive'.
static Lock aliveLock;

// System-specific logic to re-try the waiting (e.g. when a new process should be added to the list
// of waiting processes).
static void systemNewProc();

// System-specific waiting logic. Returns the process id that terminated.
static bool systemWaitProc(ProcId &proc, int &result);

// Wait for any process we're monitoring to terminate, and handle that event.
// Assumes that it is not called from multiple threads. Use 'waitFor' to ensure this.
void waitProc() {
	ProcId exited;
	int result;
	if (!systemWaitProc(exited, result))
		return;

	Process *p = null;

	{
		Lock::Guard z(aliveLock);
		ProcMap::const_iterator i = alive.find(exited);

		if (i == alive.end())
			return;

		p = i->second;
		alive.erase(i);
	}

	if (!p)
		return;

	p->terminated(result);

	if (p->owner)
		p->owner->terminated(p, result);
}

ProcessCallback::~ProcessCallback() {}

class WaitCond {
public:
	WaitCond() : next(null), sema(0), manager(false) {}

	// Next in the list.
	WaitCond *next;

	// Semaphore used for waiting.
	Sema sema;

	// Is this the manager at the moment?
	bool manager;

	// Are we done?
	virtual bool done() = 0;
};

// All conditions currently being waited for.
static WaitCond *waiters = null;

// Lock for the waiters.
static Lock waitersLock;

// Wait until 'cond' returns true. The condition may be evaluated both when 'waitLock' is held and
// when it is not held.
void waitFor(WaitCond &cond) {
	{
		Lock::Guard z(waitersLock);

		if (cond.done())
			return;

		// Need to wait.
		cond.manager = waiters == null;
		cond.next = waiters;
		waiters = &cond;
	}

	if (!cond.manager) { // Valgrind indicates this might needs synchronization, is this true?
		// Sleep until it is either our turn to become the manager, or until we're done.
		cond.sema.down();
	}

	// Either, we were the manager from the start, or we have become the new manager.
	if (cond.manager) {
		// The first thread is responsible for calling 'waitProc'.
		while (true) {
			waitProc();

			Lock::Guard w(waitersLock);

			// Check the other conditions, possibly waking them.
			WaitCond **i = &waiters;
			while (*i) {
				WaitCond *curr = *i;
				if (*i == &cond) {
					// This is us, remove it!
					*i = curr->next;
				} else if ((*i)->done()) {
					// Done. Remove it.
					*i = curr->next;
					curr->sema.up();
				} else {
					// Skip ahead.
					i = &curr->next;
				}
			}

			// If we're done, we might need to hand over to someone else.
			if (cond.done()) {
				if (waiters) {
					// Pick the first one, and wake it. It will become the new master since it is
					// not done.
					waiters->manager = true;
					waiters->sema.up();
				}

				return;
			} else {
				// We removed ourselves earlier, add us back to the list.
				cond.next = waiters;
				waiters = &cond;
			}
		}
	}
}


Process::Process(const Path &file, const vector<String> &args, const Path &cwd, const Env *env, nat skipLines) :
	callback(null), file(file), args(args), cwd(cwd), env(env ? env->data() : null), skipLines(skipLines),
	process(invalidProc), owner(null), outPipe(noPipe), errPipe(noPipe), result(0), finished(false) {}

int Process::wait() {
	class Finished : public WaitCond {
	public:
		Process *me;
		Finished(Process *p) : me(p) {}
		virtual bool done() {
			Lock::Guard z(me->finishLock);
			return me->finished;
		}
	};

	Finished c(this);
	waitFor(c);

	process = invalidProc;
	return result;
}

void Process::terminated(int result) {
	{
		Lock::Guard z(finishLock);
		this->result = result;
		this->finished = true;
	}

	// Check the callback.
	if (callback)
		callback->exited(result);
}

#ifdef WINDOWS

const ProcId invalidProc = INVALID_HANDLE_VALUE;

bool Process::spawn(bool manage, OutputState *state) {
	ostringstream cmdline;
	cmdline << file;
	for (nat i = 0; i < args.size(); i++)
		cmdline << ' ' << args[i];

	DEBUG("Running " << cmdline.str(), VERBOSE);

	STARTUPINFO si;
	zeroMem(si);

	si.cb = sizeof(STARTUPINFO);
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.dwFlags |= STARTF_USESTDHANDLES;

	if (manage) {
		Pipe readStderr, writeStderr;
		createPipe(readStderr, writeStderr, true);
		si.hStdError = writeStderr;
		errPipe = readStderr;
		OutputMgr::addError(readStderr, state);

		Pipe readStdout, writeStdout;
		createPipe(readStdout, writeStdout, true);
		si.hStdOutput = writeStdout;
		outPipe = readStdout;
		OutputMgr::add(readStdout, state, skipLines);
	} else {
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	PROCESS_INFORMATION pi;
	zeroMem(pi);

	BOOL ok = CreateProcess(toS(file).c_str(),
							(char *)cmdline.str().c_str(),
							NULL,
							NULL,
							TRUE,
							CREATE_SUSPENDED,
							env,
							toS(cwd).c_str(),
							&si,
							&pi);

	if (!ok) {
		WARNING("Failed to launch " << file);
		process = invalidProc;

		if (manage) {
			CloseHandle(si.hStdError);
			CloseHandle(si.hStdOutput);
			OutputMgr::remove(errPipe);
			OutputMgr::remove(outPipe);
		}
		return false;
	}

	process = pi.hProcess;

	{
		Lock::Guard z(aliveLock);
		alive.insert(make_pair(process, this));
		systemNewProc();
	}

	// Start the process!
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);

	if (manage) {
		CloseHandle(si.hStdError);
		CloseHandle(si.hStdOutput);
	}

	return true;
}

Process::~Process() {
	if (process != invalidProc) {
		{
			Lock::Guard z(aliveLock);
			alive.erase(process);
		}
		CloseHandle(process);

		if (outPipe != noPipe)
			OutputMgr::remove(outPipe);

		if (errPipe != noPipe)
			OutputMgr::remove(errPipe);
	}

	delete callback;
}

static String getEnv(const char *name) {
	char buf[512] = { 0 };
	GetEnvironmentVariable(name, buf, 512);
	return buf;
}

static Lock comSpecLock;
static Path comSpecPath;
static bool comSpecInitialized;

Process *shellProcess(const String &command, const Path &cwd, const Env *env, nat skip) {
	// Note: It seems like initialization of static member variables is not atomic on VS2008...
	{
		Lock::Guard z(comSpecLock);
		if (!comSpecInitialized) {
			comSpecPath = Path(getEnv("ComSpec"));
			comSpecInitialized = true;
		}
	}

	vector<String> args(3);
	args[0] = "/S";
	args[1] = "/C";
	args[2] = "\"";
	args[2].reserve(command.size() + 2);
	args[2] += command;
	args[2].push_back('\"');

	// Safe to access comSpecPath here, we know it is initialized at this point.
	return new Process(comSpecPath, args, cwd, env, skip);
}

// Event notified whenever we have a new process that we want to wait for. Used to cause the systemWaitProc to restart.
static HANDLE selfEvent = NULL;

static void systemNewProc() {
	Lock::Guard z(aliveLock);

	if (!selfEvent)
		return;

	SetEvent(selfEvent);
}

static bool systemWaitProc(ProcId &proc, int &code) {
	nat size = 0;
	ProcId *ids = null;

	{
		Lock::Guard z(aliveLock);

		if (!selfEvent)
			selfEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		ids = new ProcId[1 + alive.size()];
		ids[size++] = selfEvent;
		for (ProcMap::const_iterator i = alive.begin(), end = alive.end(); i != end; ++i) {
			if (!i->second->terminated()) {
				ids[size++] = i->first;
			}
		}
	}

	if (size <= 1) {
		delete []ids;
		return false;
	}

	DWORD result = WaitForMultipleObjects(size, ids, FALSE, INFINITE);

	nat id = 0;
	if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + size) {
		id = result - WAIT_OBJECT_0;
	} else if (result >= WAIT_ABANDONED_0 && result < WAIT_ABANDONED_0 + size) {
		id = result - WAIT_ABANDONED_0;
	} else {
		WARNING("Failed to call WaitForMultipleObjects while waiting: " << GetLastError());
		exit(10);
	}

	if (id == 0) {
		// It was the 'selfEvent'.
		delete []ids;
		return false;
	}

	proc = ids[id];
	delete []ids;

	DWORD c = 1;
	GetExitCodeProcess(proc, &c);
	code = int(c);
	return true;
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

bool Process::spawn(bool manage, OutputState *state) {
	nat argc = args.size() + 1;
	char **argv = new char *[argc + 1];

	argv[0] = createStr(toS(file));

	for (nat i = 0; i < args.size(); i++) {
		argv[i + 1] = createStr(args[i]);
	}

	argv[argc] = null;

	Pipe writeStderr, writeStdout;
	if (manage) {
		Pipe readStderr, readStdout;
		// Don't create shareable handles, we dup2() them anyway:
		createPipe(readStderr, writeStderr, false);
		errPipe = readStderr;
		OutputMgr::addError(readStderr, state);

		// Don't create shareable handles, we dup2() them anyway:
		createPipe(readStdout, writeStdout, false);
		outPipe = readStdout;
		OutputMgr::add(readStdout, state);
	}

	// Prepare everything so we do not have to do potential mallocs in the child.
	String wdStr = toS(cwd);
	const char *wdCStr = wdStr.c_str();

	sigset_t waitFor;
	sigemptyset(&waitFor);
	sigaddset(&waitFor, SIGCONT);

	// Make sure we do not consume SIGCONT too early in the child.
	sigset_t oldMask;
	pthread_sigmask(SIG_BLOCK, &waitFor, &oldMask);

	pid_t child = fork();
	if (child == 0) {
		if (chdir(wdCStr)) {
			perror("Failed to chdir: ");
			exit(1);
		}

		// Redirect stderr/stdout.
		if (manage) {
			close(STDERR_FILENO);
			dup2(writeStderr, STDERR_FILENO);
			close(writeStderr);

			close(STDOUT_FILENO);
			dup2(writeStdout, STDOUT_FILENO);
			close(writeStdout);
		}

		// Wait until our parent is ready...
		int signal;
		do {
			if (sigwait(&waitFor, &signal)) {
				perror("Sigwait: ");
				exit(4);
			}
		} while (signal != SIGCONT);

		if (env) {
			execve(argv[0], argv, (char **)env);
		} else {
			execv(argv[0], argv);
		}

		perror("Failed to exec: ");
		exit(1);
	}

	// Restore mask.
	pthread_sigmask(SIG_SETMASK, &oldMask, null);

	process = child;

	{
		Lock::Guard z(aliveLock);
		alive.insert(make_pair(process, this));
		systemNewProc();
	}

	// Start child again.
	kill(child, SIGCONT);

	if (manage) {
		// Close our ends of the pipes.
		closePipe(writeStderr);
		closePipe(writeStdout);
	}

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
		{
			Lock::Guard z(aliveLock);
			alive.erase(process);
		}

		if (outPipe != noPipe)
			OutputMgr::remove(outPipe);

		if (errPipe != noPipe)
			OutputMgr::remove(errPipe);
	}

	delete callback;
}

Process *shellProcess(const String &command, const Path &cwd, const Env *env, nat skip) {
	vector<String> args;
	args.push_back("-c");
	args.push_back(command);

	return new Process(Path("/bin/sh"), args, cwd, env, skip);
}

static void systemNewProc() {
	// Not needed on Linux, waitpid will catch the new child anyway.
}

static bool systemWaitProc(ProcId &proc, int &result) {
	{
		Lock::Guard z(aliveLock);
		if (alive.empty())
			return false;
	}

	while (true) {
		int status;
		proc = waitpid(-1, &status, 0);
		if (proc < 0) {
			perror("waitpid: ");
			continue;
		}

		if (WIFEXITED(status)) {
			result = WEXITSTATUS(status);
			break;
		} else if (WIFSIGNALED(status)) {
			result = -WTERMSIG(status);
			break;
		}
	}

	return true;
}

#endif


ProcGroup::ProcGroup(nat limit, OutputState &state) : state(state), limit(limit), failed(false) {
	if (limit == 0)
		limit = 1;
}

ProcGroup::~ProcGroup() {

	// Wait until all processes terminated.
	class Empty : public WaitCond {
	public:
		ProcGroup *me;
		Empty(ProcGroup *me) : me(me) {}
		virtual bool done() {
			return me->our.empty();
		}
	};

	Empty c(this);
	waitFor(c);


	// Remove any remaining processes, in case someting went wrong.
	Lock::Guard z(aliveLock);
	for (set<Process *>::iterator i = our.begin(), end = our.end(); i != end; ++i) {
		alive.erase((*i)->process);
		delete *i;
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

	class CanSpawn : public WaitCond {
	public:
		ProcGroup *me;
		CanSpawn(ProcGroup *me) : me(me) {}
		virtual bool done() {
			return me->canSpawn() || me->failed;
		}
	};

	CanSpawn c(this);
	waitFor(c);

	if (failed) {
		delete p;
		return false;
	}

	our.insert(p);
	if (!p->spawn(true, &state)) {
		our.erase(p);
		return false;
	}

	// Make the output look more logical to the user when running on one thread.
	if (limit == 1 || procLimit == 1) {
		return wait();
	}

	return true;
}

bool ProcGroup::wait() {
	class Failed : public WaitCond {
	public:
		ProcGroup *me;
		Failed(ProcGroup *me) : me(me) {}
		virtual bool done() {
			return me->failed || me->our.empty();
		}
	};

	Failed c(this);
	waitFor(c);

	return !failed;
}

void ProcGroup::terminated(Process *p, int result) {
	// Not our process?
	if (our.erase(p) == 0)
		return;

	if (result != 0)
		failed = true;

	delete p;
}



int exec(const Path &binary, const vector<String> &args, const Path &cwd, const Env *env) {
	Process p(binary, args, cwd, env, 0);
	if (!p.spawn())
		return 1;
	return p.wait();
}

// Run a command through a shell.
int shellExec(const String &command, const Path &cwd, const Env *env, nat skip) {
	Process *p = shellProcess(command, cwd, env, skip);
	int r = 1;
	if (p->spawn())
		r = p->wait();
	delete p;
	return r;
}
