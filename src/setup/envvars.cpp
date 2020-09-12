#include "std.h"
#include "envvars.h"
#include "process.h"
#include <algorithm>

namespace setup {

#ifdef WINDOWS
#pragma warning (disable: 4996) // for getenv()
#endif

	vector<String> parseVals(const String &vals) {
		vector<String> result;
		String part;
		istringstream iss(vals);
		while (getline(iss, part, ';'))
			result.push_back(part);
		return result;
	}

	EnvVars parseEnv(istream &from) {
		EnvVars result;
		String line;

		// Skip the leading text.
		while (getline(from, line)) {
			if (line.size() > 3 && line.substr(line.size() - 3) == "set")
				break;
		}

		// Parse environment variables.
		while (getline(from, line)) {
			size_t eq = line.find('=');
			if (eq == String::npos)
				break;

			size_t end = line.size();
			if (line[end - 1] == '\r') {
				end--;
			}
			result[line.substr(0, eq)] = parseVals(line.substr(eq + 1, end - eq - 1));
		}

		return result;
	}

	void subtract(vector<String> &from, const vector<String> &remove) {
		class IfPresent {
		public:
			const vector<String> &remove;
			IfPresent(const vector<String> &remove) : remove(remove) {}

			bool operator ()(const String &v) const {
				return std::find(remove.begin(), remove.end(), v) != remove.end();
			}
		};

		vector<String>::iterator i = std::remove_if(from.begin(), from.end(), IfPresent(remove));
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

	ostream &operator <<(ostream &to, const vector<String> &str) {
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

}
