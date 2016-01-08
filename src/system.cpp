#include "std.h"
#include "system.h"

#ifdef WINDOWS

int exec(const Path &binary, const vector<String> &args) {
	ostringstream cmdline;

	cmdline << '"';
	cmdline << binary;
	for (nat i = 0; i < args.size(); i++) {
		cmdline << ' ' << args[i];
	}
	cmdline << '"';

	PVAR(Path::cwd());
	PLN("Running " << cmdline.str());
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

#error "Implement support for Linux/UNIX!"

#endif
