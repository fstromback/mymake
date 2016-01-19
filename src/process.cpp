#include "std.h"
#include "process.h"

Process::Process(const Path &file, const vector<String> &args, const Path &cwd, const Env *env) :
	file(file), args(args), cwd(cwd), env(env), process(invalidProc), owner(null) {}

#ifdef WINDOWS

const ProcId invalidProc = INVALID_HANDLE_VALUE;

bool Process::spawn() {
	ostringstream cmdline;
	cmdline << binary;
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

	BOOL ok = CreateProcess(toS(binary).c_str(),
							(char *)cmdline.str().c_str(),
							NULL,
							NULL,
							TRUE,
							0,
							env ? env->data() : NULL,
							toS(cwd).c_str(),
							&si,
							&pi);

	if (!ok) {
		WARNING("Failed to launch " << binary);
		process = invalidProc;
		return false;
	}

	process = pi.hProcess;
	CloseHandle(pi.hThread);

	return true;
}

Process::~Process() {
	if (process != invalidProc)
		CloseHandle(process);
}

int Process::wait() {
	if (process == invalidProc)
		return 1;

	DWORD code = 1;
	WaitForSingleObject(process, INFINITE);
	GetExitCodeProcess(process, &code);

	CloseHandle(process);
	process = invalidProc;

	return int(code);
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

		if (env) {
			execve(argv[0], argv, (char **)env->data());
		} else {
			execv(argv[0], argv);
		}
		PLN("Failed to exec!");
		exit(1);
	}

	// Clean up...
	for (nat i = 0; i < argc; i++) {
		delete []argv[i];
	}
	delete []argv;

	process = child;

	return true;
}

Process::~Process() {
	// We can not clean up a child without doing 'wait'. We let the system clear this when mymake
	// exits.
}

int Process::wait() {
	if (process == invalidProc)
		return 1;

	int status;
	waitpid(process, &status, 0);
	process = invalidProc;

	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}

	return 1;
}

Process *shellProcess(const String &command, const Path &cwd, const Env *env) {
	vector<String> args;
	args.push_back("-c");
	args.push_back(command);

	return new Process(Path("/bin/sh"), args, cwd, env);
}

#endif


ProcGroup::ProcGroup() {}

void ProcGroup::setLimit(nat l) {
	if (l == 0)
		return;
	limit = l;
}

int ProcGroup::wait() {
	// Someone terminated?
	if (!errors.empty()) {
		int r = errors.front(); errors.pop();
		return r;
	}

	// No need to wait?
	if (alive.size() < limit) {
		return 0;
	}

	// Wait until someone exited.
	ProcId proc;
	int status = 0;
	waitChild(proc, status);

	// See their exit code and report appropriatly.
	map<ProcId, Process *>::iterator i = alive.find(proc);
	if (i == alive.end()) {
		WARNING("ProcGroup::wait used alongside Process::wait!");
		return 0;
	}

	// We may not be the rightful owner...
	Process *p = i->second;
	ProcGroup *owner = p->owner;
	delete p;
	alive.erase(i);

	if (!owner) {
		WARNING("Found a child without proper owner...");
		return 0;
	}

	if (status != 0) {
		owner->errors.push(status);
	}

	if (!errors.empty()) {
		int r = errors.front(); errors.pop();
		return r;
	}

	return 0;
}

void ProcGroup::spawn(Process *proc) {
	proc->owner = this;
	proc->spawn();

	alive.insert(make_pair(proc->process, proc));
}

nat ProcGroup::limit = 1;

map<ProcId, Process *> ProcGroup::alive;

#ifdef WINDOWS


#else

void ProcGroup::waitChild(ProcId &process, int &e) {
	int status;
	do {
		process = waitpid(-1, &status, 0);
	} while (!WIFEXITED(status));
	e = WEXITSTATUS(status);
}

#endif


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
