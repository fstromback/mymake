#include "std.h"
#include "system.h"

#ifdef WINDOWS

int exec(const Path &binary, const vector<String> &args) {
	ostringstream cmdline;

	// cmdline << '"';
	cmdline << binary;
	for (nat i = 0; i < args.size(); i++) {
		cmdline << ' ' << args[i];
	}
	//cmdline << '"';

	DEBUG("Running " << cmdline.str(), VERBOSE);
	return system(cmdline.str().c_str());
}

const char envSeparator = ';';

String getEnv(const String &key) {
	DWORD required = GetEnvironmentVariable(key.c_str(), NULL, 0);
	if (required == 0)
		return "";

	char *buffer = new char[required + 10];
	GetEnvironmentVariable(key.c_str(), buffer, required + 1);

	String r = buffer;
	delete []buffer;
	return r;
}

void setEnv(const String &key, const String &v) {
	DEBUG("Env: " << key << "=" << v, VERBOSE);
	SetEnvironmentVariable(key.c_str(), v.empty() ? null : v.c_str());
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

int exec(const Path &binary, const vector<String> &args) {
	nat argc = args.size() + 1;
	char **argv = new char *[argc + 1];

	argv[0] = createStr(toS(binary));

	for (nat i = 0; i < args.size(); i++) {
		argv[i + 1] = createStr(args[i]);
	}

	argv[argc] = null;

	pid_t child = fork();
	if (child == 0) {
		execv(argv[0], argv);
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

const char envSeparator = ':';

String getEnv(const String &key) {
	return getenv(key.c_str());
}

void setEnv(const String &key, const String &v) {
	if (v.empty()) {
		unsetenv(key.c_str());
	} else {
		setenv(key.c_str(), v.c_str(), 1);
	}
}

#endif
