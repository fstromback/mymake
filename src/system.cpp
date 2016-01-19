#include "std.h"
#include "system.h"
#include "env.h"

#ifdef WINDOWS

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

static char *createStr(const String &str) {
	char *n = new char[str.size() + 1];
	memcpy(n, str.c_str(), str.size() + 1);
	return n;
}

const char envSeparator = ':';

EnvData allocEnv(nat entries, nat) {
	char **data = new char *[entries + 1];
	for (nat i = 0; i <= entries; i++)
		data[i] = null;

	return data;
}

void freeEnv(EnvData data) {
	char **d = (char **)data;
	for (nat pos = 0; d[pos]; pos++) {
		delete []d[pos];
	}

	delete[] d;
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

	d[pos] = createStr(data);

	pos++;
}

static nat countEntries(char **in) {
	nat at = 0;
	while (in[at])
		at++;
	return at;
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
