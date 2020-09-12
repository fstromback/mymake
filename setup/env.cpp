#include "env.h"
#include "process.h"
#include <algorithm>

vector<string> parseVals(const string &vals) {
	vector<string> result;
	string part;
	istringstream iss(vals);
	while (getline(iss, part, ';'))
		result.push_back(part);
	return result;
}

EnvVars parseEnv(istream &from) {
	EnvVars result;
	string line;

	// Skip the leading text.
	while (getline(from, line)) {
		if (line.size() > 3 && line.substr(line.size() - 3) == "set")
			break;
	}

	// Parse environment variables.
	while (getline(from, line)) {
		size_t eq = line.find('=');
		if (eq == string::npos)
			break;

		result[line.substr(0, eq)] = parseVals(line.substr(eq + 1));
	}

	return result;
}

EnvVars captureEnv(const string &command) {
	string cmd = getenv("comspec");
	istringstream in("set\n");
	stringstream out;
	runProcess(cmd, "/K " + command, in, out);

	return parseEnv(out);
}

void subtract(vector<string> &from, const vector<string> &remove) {
	class IfPresent {
	public:
		const vector<string> &remove;
		IfPresent(const vector<string> &remove) : remove(remove) {}

		bool operator ()(const string &v) const {
			return std::find(remove.begin(), remove.end(), v) != remove.end();
		}
	};

	vector<string>::iterator i = std::remove_if(from.begin(), from.end(), IfPresent(remove));
	from.erase(i, from.end());
}

EnvVars subtract(const EnvVars &full, const EnvVars &remove) {
	EnvVars result = full;
	for (EnvVars::const_iterator i = remove.begin(); i != remove.end(); ++i) {
		EnvVars::iterator found = result.find(i->first);
		if (found == result.end())
			continue;

		subtract(found->second, i->second);
		if (found->second.empty())
			result.erase(found);
	}
	return result;
}

ostream &operator <<(ostream &to, const vector<string> &str) {
	if (str.size() == 0)
		return to;

	to << str[0];
	for (size_t i = 1; i < str.size(); i++) {
		to << ';' << str[i];
	}
	return to;
}

ostream &operator <<(ostream &to, const EnvVars &vars) {
	for (EnvVars::const_iterator i = vars.begin(); i != vars.end(); ++i) {
		to << i->first << "=" << i->second << endl;
	}
	return to;
}
