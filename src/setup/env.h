#pragma once

namespace setup {

	/**
	 * Captured environment variables.
	 */
	typedef map<String, vector<String>> EnvVars;

	// Parse environment variables from executing "set" in "cmd.exe".
	EnvVars parseEnv(istream &from);

	// Subtract all parts from one set from another.
	EnvVars subtract(const EnvVars &full, const EnvVars &remove);

	// Print them.
	ostream &operator <<(ostream &to, const vector<String> &str);
	ostream &operator <<(ostream &to, const EnvVars &vars);

}
