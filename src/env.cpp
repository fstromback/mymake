#include "std.h"
#include "env.h"
#include "system.h"

Env::Env(const Config &config) {
	vector<String> ops = config.getArray("env");

	EnvMap vars;
	for (nat i = 0; i < ops.size(); i++) {
		alter(ops[i], vars);
	}

	for (EnvMap::iterator i = vars.begin(); i != vars.end(); ++i) {
		setEnv(i->first, i->second);
	}
}

Env::~Env() {
	for (EnvMap::iterator i = restore.begin(); i != restore.end(); ++i) {
		setEnv(i->first, i->second);
	}
}

void Env::alter(const String &str, EnvMap &vars) {
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

void Env::alter(const String &key, const String &value, Mode mode, EnvMap &vars) {
	if (restore.count(key) == 0)
		restore[key] = getEnv(key);

	if (vars.count(key) == 0)
		vars[key] = restore[key];

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
