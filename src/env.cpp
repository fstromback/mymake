#include "std.h"
#include "env.h"
#include "system.h"
#include "config.h"


Env::Env() : osData(null) {}

Env::Env(const Env &o) : vars(o.vars), osData(null) {}

Env &Env::operator =(const Env &o) {
	if (osData)
		freeEnv(osData);
	osData = null;

	vars = o.vars;
	return *this;
}

Env::~Env() {
	if (osData)
		freeEnv(osData);
}

Env Env::current() {
	Env result;
	result.osData = getEnv();
	result.vars = toMap(result.osData);
	// Note: 'osData' might be destroyed immediately here. That is fine, since we will generally
	// modify the osData before we use it anyway, which requires a re-allocation anyway.
	// Most likely RVO will handle this nicely, especially after C++11 or so.
	return result;
}

Env Env::update(const Env &original, const Config &config) {
	Env result(original);
	// Note: copy ctor destroys contents on 'osData'. We don't have to invalidate anything in
	// 'result' when modifying it!

	vector<String> ops = config.getArray("env");
	for (nat i = 0; i < ops.size(); i++) {
		result.alter(ops[i]);
	}

	return result;
}

bool Env::get(const String &key, String &out) const {
	Map::const_iterator i = vars.find(key);
	if (i == vars.end())
		return false;

	out = i->second;
	return true;
}

EnvData Env::data() const {
	if (!osData)
		osData = toData();
	return osData;
}

/**
 * Helpers
 */

bool Env::KeyCompare::operator () (const String &a, const String &b) const {
	return envLess(a.c_str(), b.c_str());
}

void Env::alter(const String &key, const String &value, Mode mode) {
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

void Env::alter(const String &str) {
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

	alter(key, val, mode);
}

Env::Map Env::toMap(EnvData src) {
	Map result;
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

EnvData Env::toData() const {
	nat totalLen = 0;
	for (Map::const_iterator i = vars.begin(), end = vars.end(); i != end; ++i) {
		totalLen += i->first.size() + i->second.size() + 1;
	}

	EnvData r = allocEnv(vars.size(), totalLen);

	nat pos = 0;
	for (Map::const_iterator i = vars.begin(), end = vars.end(); i != end; ++i) {
		writeEnv(r, pos, i->first + "=" + i->second);
	}

	return r;
}

ostream &operator <<(ostream &to, const Env &env) {
	for (Env::Map::const_iterator i = env.vars.begin(); i != env.vars.end(); ++i) {
		to << endl << i->first << "=" << i->second;
	}
	return to;
}
