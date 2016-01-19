#include "std.h"
#include "system.h"
#include "env.h"

#ifdef WINDOWS

int exec(const Path &binary, const vector<String> &args, const Path &cwd, const Env *env) {
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
		return 1;
	}

	DWORD code = 1;
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &code);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return int(code);
}

static String getEnv(const char *name) {
	char buf[512] = { 0 };
	GetEnvironmentVariable(name, buf, 512);
	return buf;
}

int shellExec(const String &command, const Path &cwd, const Env *env) {
	String shell = getEnv("ComSpec");

	vector<String> args;
	args.push_back("/S");
	args.push_back("/C");
	args.push_back("\"" + command + "\"");
	return exec(Path(shell), args, cwd, env);
}

const char envSeparator = ';';

EnvData allocEnv(nat entries, nat totalCount) {
	nat count = entries + totalCount + 1;
	return new char[count];
}

void freeEnv(EnvData data) {
	char *d = (char *)data;
	delete []d;
}

bool readEnv(EnvData env, nat &pos, String &data) {
	char *d = (char *)env;
	if (d[pos] == 0)
		return false;

	data = d + pos;
	pos += data.size() + 1;

	return true;
}

void writeEnv(EnvData env, nat &pos, const String &data) {
	char *d = (char *)env;

	for (nat i = 0; i < data.size(); i++) {
		d[pos + i] = data[i];
	}
	d[pos + data.size()] = 0;

	pos += data.size() + 1;

	// Set additional null if this is the last entry.
	d[pos] = 0;
}

EnvData getEnv() {
	char *d = GetEnvironmentStrings();

	nat pos = 0;
	while (d[pos]) {
		pos += strlen(d + pos) + 1;
	}

	nat count = pos + 1;
	char *r = new char[count];
	memcpy_s(r, count, d, count);

	FreeEnvironmentStrings(d);
	return r;
}

bool envLess(const char *a, const char *b) {
	return _stricmp(a, b) < 0;
}

#else
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

char *createStr(const String &str) {
	char *n = new char[str.size() + 1];
	memcpy(n, str.c_str(), str.size() + 1);
	return n;
}

int exec(const Path &binary, const vector<String> &args, const Path &cwd, const Env *env) {
	nat argc = args.size() + 1;
	char **argv = new char *[argc + 1];

	argv[0] = createStr(toS(binary));

	for (nat i = 0; i < args.size(); i++) {
		argv[i + 1] = createStr(args[i]);
	}

	argv[argc] = null;

	pid_t child = fork();
	if (child == 0) {
		chdir(toS(cwd).c_str());
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

	int status;
	waitpid(child, &status, 0);

	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}

	// Some problem...
	return 1;
}

int shellExec(const String &command, const Path &cwd, const Env *env) {
	vector<String> args;
	args.push_back("-c");
	args.push_back(command);
	return exec(Path("/bin/sh"), args, cwd, env);
}

const char envSeparator = ':';

EnvData allocEnv(nat entries, nat) {
	char **data = new char *[entries + 1];
	for (nat i = 0; i <= entries; i++)
		data[i] = null;

	return data;
}

void freeEnv(EnvData data) {
	for (nat pos = 0; data[pos]; pos++) {
		delete []data[pos];
	}

	delete[] data;
}

bool readEnv(EnvData env, nat &pos, String &data) {
	char **d = (char **)env;
	if (d[pos] == null)
		return false;

	data = d[pos++];

	return true;
}

void writeEnv(EnvData env, nat &pos, const String &data) {
	char **d = (char **)env;

	data[pos] = createStr(data);

	pos++;
}

static nat countEntries(char **in) {
	nat at = 0;
	while (in[at])
		at++;
	return at + 1;
}

EnvData getEnv() {
	nat entries = countEntries(environ);

	char **data = new char *[entries + 1];
	data[entries] = null;

	for (nat i = 0; i < entries; i++) {
		nat len = strlen(environ[i]);
		data[i] = new char[len + 1];
		memcpy(data[i], environ[i], len + 1);
	}

	return data;
}

bool envLess(const char *a, const char *b) {
	return strcmp(a, b) < 0;
}

#endif
