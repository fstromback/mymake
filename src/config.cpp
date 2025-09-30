#include "std.h"
#include "config.h"

const String localConfig = ".mymake";
const String projectConfig = ".myproject";

// Known bug: can not compile stuff in the root directory...
Path findConfig() {
	Path globalConfig = Path::config();

	for (Path dir = Path::cwd(); !dir.isEmpty(); dir = dir.parent()) {
		// Skip home directory, where the global configuration is located.
		if (dir == globalConfig)
			continue;

		// Search for a configuration...
		if ((dir + projectConfig).exists())
			return dir;

		if ((dir + localConfig).exists()) {
			// Parent directory might contain a valid configuration!
			if ((dir.parent() + projectConfig).exists())
				return dir.parent();

			return dir;
		}
	}

	// Default to cwd.
	return Path::cwd();
}

Path defaultGlobalConfig() {
	return Path::config() + "mymake.conf";
}

/**
 * Make config.
 */

MakeConfig::MakeConfig() {}

const set<String> &MakeConfig::options() const {
	return allOptions;
}

bool MakeConfig::load(const Path &path) {
	ifstream src(toS(path).c_str());
	nat initSize = sections.size();

	String line;
	while (getline(src, line)) {
		if (line.empty())
			continue;

		if (line[0] == '#')
			continue;

		if (line[0] == '[') {
			parseSection(line);
		} else {
			parseAssignment(line, initSize);
		}

	}

	return true;
}

void MakeConfig::parseSection(String line) {
	line = trim(line);
	if (line[0] != '[' || line[line.size() - 1] != ']')
		return;
	line = line.substr(1, line.size() - 2);
	vector<String> parts = split(line, ",");

	Section s;

	for (nat i = 0; i < parts.size(); i++) {
		String t = trim(parts[i]);
		if (!t.empty() && t[0] == '!') {
			s.exclude << t.substr(1);
		} else {
			s.options << t;
		}
	}

	allOptions.insert(s.options.begin(), s.options.end());
	allOptions.insert(s.exclude.begin(), s.exclude.end());

	sections << s;
}

void MakeConfig::parseAssignment(String line, nat initialSize) {
	nat eq = line.find('=');
	if (eq == String::npos)
		return;

	Assignment a = {
		mSet,
		line.substr(0, eq),
		line.substr(eq + 1),
	};

	char last = a.key[a.key.size() - 1];
	if (last == '+') {
		a.key = a.key.substr(0, a.key.size() - 1);
		a.mode = mAppend;
	}

	if (sections.size() <= initialSize) {
		Section s;
		sections << s;
	}

	sections.back().assignments << a;
}

static bool allOf(const set<String> &of, const set<String> &in) {
	for (set<String>::const_iterator i = of.begin(); i != of.end(); ++i) {
		if (in.count(*i) == 0)
			return false;
	}

	return true;
}

static bool noneOf(const set<String> &of, const set<String> &in) {
	for (set<String>::const_iterator i = of.begin(); i != of.end(); ++i) {
		if (in.count(*i) != 0)
			return false;
	}
	return true;
}

void MakeConfig::apply(set<String> options, Config &to) const {

#ifdef WINDOWS
	options.insert("windows");
#else
	options.insert("unix");
#endif

	for (nat i = 0; i < sections.size(); i++) {
		const Section &s = sections[i];

		if (!allOf(s.options, options))
			continue;
		if (!noneOf(s.exclude, options))
			continue;

		for (nat i = 0; i < s.assignments.size(); i++) {
			const Assignment &a = s.assignments[i];

			switch (a.mode) {
			case mAppend:
				to.add(a.key, a.value);
				break;
			case mSet:
				if (a.value.empty())
					to.clear(a.key);
				else
					to.set(a.key, a.value);
				break;
			}
		}
	}
}

ostream &operator <<(ostream &to, const MakeConfig &c) {
	for (nat i = 0; i < c.sections.size(); i++) {
		const MakeConfig::Section &s = c.sections[i];
		to << endl;
		to << "[" << join(s.options, ",") << "]";

		for (nat i = 0; i < s.assignments.size(); i++) {
			const MakeConfig::Assignment &a = s.assignments[i];
			to << endl << a.key;
			switch (a.mode) {
			case MakeConfig::mAppend:
				to << "+=";
				break;
			case MakeConfig::mSet:
				to << '=';
				if (a.value.empty())
					to << "<erase>";
				break;
			}
			to << a.value;
		}
	}

	return to;
}


/**
 * Config.
 */

Config::Config() : env(Env::empty()) {
	data.insert(make_pair("library", Value()));
}

void Config::set(const String &k, const String &v) {
	data[k] = vector<String>(1, v);
}

void Config::add(const String &k, const String &v) {
	data[k] << v;
}

void Config::add(const String &k, const vector<String> &v) {
	vector<String> &to = data[k];
	for (nat i = 0; i < v.size(); i++)
		to << v[i];
}

void Config::clear(const String &k) {
	data[k].clear();
}

bool Config::has(const String &k) const {
	return data.count(k) > 0;
}

String Config::getStr(const String &k, const String &d) const {
	Data::const_iterator i = data.find(k);
	if (i == data.end())
		return d;

	return join(i->second, " ");
}

vector<String> Config::getArray(const String &k, const vector<String> &d) const {
	Data::const_iterator i = data.find(k);
	if (i == data.end())
		return d;

	return i->second;
}

bool Config::getBool(const String &k, bool def) const {
	String r = getStr(k, def ? "yes" : "no");
	return r == "yes";
}

String Config::getVars(const String &key, const SpecialMap &special) const {
	return expandVars(getStr(key), special);
}

String Config::expandVars(const String &into, const SpecialMap &special) const {
	std::ostringstream to;

	nat start = String::npos;

	for (nat i = 0; i < into.size(); i++) {
		if (start == String::npos) {
			if (into[i] == '<') {
				start = i + 1;
			} else {
				to << into[i];
			}
		} else {
			if (into[i] == ' ') {
				for (nat j = start - 1; j <= i; j++)
					to << into[j];
				start = String::npos;
			} else if (into[i] == '>') {
				join(to, replacement(into.substr(start, i - start), special), " ");
				start = String::npos;
			}
		}
	}

	return to.str();
}

static bool hasPrefix(const String &str, const char *prefix) {
	for (nat i = 0; i < str.size(); i++) {
		if (prefix[i] == 0)
			return true;
		if (prefix[i] != str[i])
			return false;
	}
	return prefix[str.size()] == 0;
}

vector<String> Config::replacement(String var, const SpecialMap &special) const {
	nat star = var.find('*');
	if (star != String::npos) {
		return addStr(join(replacement(var.substr(0, star), special), " "),
					replacement(var.substr(star + 1), special));
	}

	nat pipe = var.find('|');
	String op;
	if (pipe != String::npos) {
		op = var.substr(0, pipe);
		var = var.substr(pipe + 1);
	}

	{
		SpecialMap::const_iterator i = special.find(var);
		if (i != special.end()) {
			pair<bool, String> r = applyFn(op, i->second);
			if (r.first)
				return vector<String>(1, r.second);
			else
				return vector<String>();
		}
	}

	// Handle built-in special cases.
	if (var == "includes") {
		return addStr(getVars("includeCl", special), replacement("path|include", special));
	} else if (var == "libs") {
		return addStr(getVars("localLibraryCl", special), replacement("path|localLibrary", special))
			+ addStr(getVars("libraryCl", special), replacement("path|library", special));
	} else if (hasPrefix(var, "env:")) {
		// Special case for environment variables:
		String result;
		if (env.get(var.substr(4), result))
			return vector<String>(1, result);
		return vector<String>();
	}

	{
		Data::const_iterator d = data.find(var);
		if (d != data.end()) {
			vector<String> data = d->second;
			for (nat i = 0; i < data.size(); i++) {
				data[i] = expandVars(data[i], special);
			}
			return applyFn(op, data);
		}
	}

	return vector<String>();
}

vector<String> Config::applyFn(const String &op, const vector<String> &src) const {
	vector<String> r;
	for (nat i = 0; i < src.size(); i++) {
		pair<bool, String> v = applyFn(op, src[i]);
		if (v.first)
			r << v.second;
	}
	return r;
}

pair<bool, String> Config::applyFn(const String &op, const String &src) const {
	if (op == "title") {
		return make_pair(true, Path(src).title());
	} else if (op == "titleNoExt") {
		return make_pair(true, Path(src).titleNoExt());
	} else if (op == "noExt") {
		Path p(src);
		p.makeExt("");
		return make_pair(true, toS(p));
	} else if (op == "path") {
		return make_pair(true, toS(Path(src)));
	} else if (op == "buildpath") {
		return make_pair(true, toS(Path(getVars("buildDir")) + Path(src)));
	} else if (op == "execpath") {
		return make_pair(true, toS(Path(getVars("execDir")) + Path(src)));
	} else if (op == "dir") {
		Path p(src);
		if (p.isEmpty())
			return make_pair(false, String());
		p = p.parent();
		if (p.isEmpty())
			return make_pair(false, String());
		return make_pair(true, toS(p));
	} else if (op == "if") {
		return make_pair(true, String());
	} else if (op.empty()) {
		return make_pair(true, src);
	} else {
		WARNING("Unknown operation: " << op);
		return make_pair(true, src);
	}
}

vector<String> Config::addStr(const String &prefix, vector<String> to) const {
	for (nat i = 0; i < to.size(); i++)
		to[i] = prefix + to[i];
	return to;
}


ostream &operator <<(ostream &to, const Config &c) {
	for (Config::Data::const_iterator i = c.data.begin(); i != c.data.end(); ++i) {
		to << endl << i->first << "=" << join(i->second, ",");
	}

	return to;
}
