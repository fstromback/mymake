#include "std.h"
#include "config.h"

const String localConfig = ".mymake";
const String projectConfig = ".myproject";

// Known bug: can not compile stuff in the root directory...
Path findConfig() {
	Path home = Path::home();

	for (Path dir = Path::cwd(); !dir.isEmpty(); dir = dir.parent()) {
		// Skip home directory, where the global configuration is located.
		if (dir == home)
			continue;

		// Search upwards for a configuration...
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


/**
 * Make config.
 */

MakeConfig::MakeConfig() {}

bool MakeConfig::load(const Path &path) {
	ifstream src(toS(path).c_str());

	String line;
	while (getline(src, line)) {
		if (line.empty())
			continue;

		if (line[0] == '#')
			continue;

		if (line[0] == '[') {
			parseSection(line);
		} else {
			parseAssignment(line);
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
		s.configurations << trim(parts[i]);
	}

	sections << s;
}

void MakeConfig::parseAssignment(String line) {
	nat eq = line.find('=');
	if (eq == String::npos)
		return;

	Assignment a = {
		false,
		line.substr(0, eq),
		line.substr(eq + 1),
	};

	if (a.key[a.key.size() - 1] == '+') {
		a.key = a.key.substr(0, a.key.size() - 1);
		a.append = true;
	}

	if (sections.empty()) {
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

void MakeConfig::apply(const set<String> &options, Config &to) const {
	for (nat i = 0; i < sections.size(); i++) {
		const Section &s = sections[i];

		if (!allOf(s.configurations, options))
			continue;

		for (nat i = 0; i < s.assignments.size(); i++) {
			const Assignment &a = s.assignments[i];

			if (a.append)
				to.add(a.key, a.value);
			else
				to.set(a.key, a.value);
		}
	}
}

ostream &operator <<(ostream &to, const MakeConfig &c) {
	for (nat i = 0; i < c.sections.size(); i++) {
		const MakeConfig::Section &s = c.sections[i];
		to << endl;
		to << "[" << join(s.configurations, ",") << "]";

		for (nat i = 0; i < s.assignments.size(); i++) {
			const MakeConfig::Assignment &a = s.assignments[i];
			to << endl << a.key << (a.append ? "+=" : "=") << a.value;
		}
	}

	return to;
}


/**
 * Config.
 */

void Config::set(const String &k, const String &v) {
	data[k] = vector<String>(1, v);
}

void Config::add(const String &k, const String &v) {
	data[k] << v;
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

String Config::getVars(const String &key, const map<String, String> &special) const {
	return expandVars(getStr(key), special);
}

String Config::expandVars(const String &into, const map<String, String> &special) const {
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
				to << replacement(into.substr(start, i - start), special);
				start = String::npos;
			}
		}
	}

	return to.str();
}

String Config::replacement(String var, const map<String, String> &special) const {
	nat star = var.find('*');
	if (star != String::npos) {
		PLN("Building string " << var);
		PVAR(replacement(var.substr(0, star), special));
		PVAR(var.substr(star + 1));
		PVAR(buildString(replacement(var.substr(0, star), special), var.substr(star + 1)));
		return buildString(replacement(var.substr(0, star), special), var.substr(star + 1));
	}

	nat pipe = var.find('|');
	String op;
	if (pipe != String::npos) {
		op = var.substr(0, pipe);
		var = var.substr(pipe + 1);
	}

	{
		map<String, String>::const_iterator i = special.find(var);
		if (i != special.end())
			return applyFn(op, i->second);
	}

	// Handle built-in special cases.
	if (var == "includes") {
		return buildString(getStr("includeCl"), "path|include");
	} else if (var == "libs") {
		return buildString(getStr("libraryCl"), "path|library");
	}

	{
		Data::const_iterator d = data.find(var);
		if (d != data.end()) {
			ostringstream to;
			for (nat i = 0; i < d->second.size(); i++) {
				if (i != 0)
					to << ' ';
				to << applyFn(op, d->second[i]);
			}
			return to.str();
		}
	}

	WARNING("Unknown variable in string: " << var);
	return "";
}

String Config::applyFn(const String &op, const String &src) const {
	if (op == "title") {
		return Path(src).title();
	} else if (op == "titleNoExt") {
		return Path(src).titleNoExt();
	} else if (op == "path") {
		return toS(Path(src));
	} else if (op.empty()) {
		return src;
	} else {
		WARNING("Unknown operation: " << op);
		return src;
	}
}

String Config::buildString(const String &insert, const String &arrayKey) const {
	ostringstream to;

	String var = arrayKey;
	nat pipe = var.find('|');
	String op;
	if (pipe != String::npos) {
		op = var.substr(0, pipe);
		var = var.substr(pipe + 1);
	}

	vector<String> a = getArray(var);
	for (nat i = 0; i < a.size(); i++) {
		if (i > 0)
			to << ' ';
		to << insert << applyFn(op, a[i]);
	}

	return to.str();
}

String Config::buildStringPath(const String &insert, const String &arrayKey) const {
	ostringstream to;

	vector<String> a = getArray(arrayKey);
	for (nat i = 0; i < a.size(); i++) {
		if (i > 0)
			to << ' ';
		to << insert << Path(a[i]);
	}

	return to.str();
}

ostream &operator <<(ostream &to, const Config &c) {
	for (Config::Data::const_iterator i = c.data.begin(); i != c.data.end(); ++i) {
		to << endl << i->first << "=" << join(i->second, ",");
	}

	return to;
}
