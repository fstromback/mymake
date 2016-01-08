#pragma once
#include "config.h"

/**
 * Management of the environment variables. Makes sure to restore the environment variables when
 * we're done.
 */
class Env : NoCopy {
public:
	// Set up the environment indicated in the config.
	Env(const Config &config);

	// Restore old environment.
	~Env();

private:
	// Environment variables we have altered that needs to be restored.
	typedef map<String, String> EnvMap;
	EnvMap restore;

	// Alter one variable.
	void alter(const String &line, EnvMap &vars);

	// Append mode.
	enum Mode {
		replace,
		prepend,
		append,
	};

	// Alter one variable.
	void alter(const String &key, const String &value, Mode mode, EnvMap &vars);
};
