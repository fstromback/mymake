#include "std.h"
#include "env.h"
#include "system.h"

struct KeyFn {
	bool operator () (const String &a, const String &b) const {
		return envLess(a.c_str(), b.c_str());
	}
};

typedef map<String, String, KeyFn> EnvMap;

// Append mode.
enum Mode {
	replace,
	prepend,
	append,
};

// Alter one variable.
static void alter(const String &key, const String &value, Mode mode, EnvMap &vars) {
	String &to = vars[key];

	switch (mode) {
	case replace:
		to = value;
		break;
	case prepend:
		if (!to.empty() && to[0] != envSeparator) {
			to = value + String(1, envSeparator) + to;
		} else {
			to = value + to;
		}
		break;
	case append:
		if (!to.empty() && to[to.size() - 1] != envSeparator) {
			to = to + String(1, envSeparator) + value + String(1, envSeparator);
		} else {
			to = to + value + String(1, envSeparator);
		}
		break;
	}
}

// Alter one variable.
static void alter(const String &str, EnvMap &vars) {
	nat eq = str.find('=');
	if (eq == String::npos)
		return;

	String key = str.substr(0, eq);
	String val = str.substr(eq + 1);

	Mode mode = replace;
	if (key[key.size() - 1] == '<') {
		mode = prepend;
		key = key.substr(0, key.size() - 1);
	} else if (val[0] == '>') {
		mode = append;
		val = val.substr(1);
	}

	alter(key, val, mode, vars);
}

// Convert EnvData to EnvMap.
static EnvMap asMap(EnvData src) {
	EnvMap result;
	String entry;
	nat pos = 0;
	while (readEnv(src, pos, entry)) {
		nat eq = entry.find('=');
		if (eq == String::npos)
			continue;

		result.insert(make_pair(entry.substr(0, eq), entry.substr(eq + 1)));
	}

	return result;
}

// Convert EnvMap to EnvData.
static EnvData asData(const EnvMap &vars) {
	nat totalLen = 0;
	for (EnvMap::const_iterator i = vars.begin(), end = vars.end(); i != end; ++i) {
		totalLen += i->first.size() + i->second.size() + 1;
	}

	EnvData r = allocEnv(vars.size(), totalLen);

	nat pos = 0;
	for (EnvMap::const_iterator i = vars.begin(), end = vars.end(); i != end; ++i) {
		writeEnv(r, pos, i->first + "=" + i->second);
	}

	return r;
}

Env::Env(const Config &config) {
	EnvData sys = getEnv();
	EnvMap vars = asMap(sys);
	freeEnv(sys);

	vector<String> ops = config.getArray("env");
	for (nat i = 0; i < ops.size(); i++) {
		alter(ops[i], vars);
	}

	d = asData(vars);
}

Env::~Env() {
	freeEnv(d);
}
