#pragma once
#include "system.h"

class Config;

/**
 * Storage and retrieval of environment variables.
 *
 * Produces two representations of environment variables: one dictionary that can be inspected from
 * C++ conveniently, and another system-specific representation that can be passed to 'exec' or
 * 'CreateProcess' for process creation. The system-specific representation is created on demand.
 */
class Env {
public:
	// Copy and destruction.
	Env(const Env &o);
	Env &operator =(const Env &o);
	~Env();

	// Create an empty environment block.
	static inline Env empty() {
		return Env();
	}

	// Create a copy of the environment variables currently visible to this process.
	static Env current();

	// Create a copy of the environment variables in 'original', but modify them according to the
	// given configuration.
	static Env update(const Env &original, const Config &config);

	// Get system-specific data.
	EnvData data() const;

	// Get a particular environment variable.
	bool get(const String &key, String &out) const;

private:
	// Create an empty env-block.
	Env();

	// Comparison of keys.
	struct KeyCompare {
		bool operator () (const String &a, const String &b) const;
	};

	// Storage for the environment variables. Custom comparison function to match how the current
	// OS searches for environment variables.
	typedef map<String, String, KeyCompare> Map;

	// Environment variables.
	Map vars;

	// OS representation of the data. Created on demand.
	mutable EnvData osData;

	/**
	 * Misc. helpers
	 */

	// Append mode.
	enum Mode {
		replace,
		prepend,
		append,
	};

	// Alter a single variable.
	void alter(const String &key, const String &value, Mode mode);

	// Alter a single variable, parsing 'str'.
	void alter(const String &str);

	// Convert 'EnvData' to 'Map'.
	static Map toMap(EnvData src);

	// Convert 'Map' to 'EnvData'.
	EnvData toData() const;

	// Output.
	friend ostream &operator <<(ostream &to, const Env &env);
};

// Output.
ostream &operator <<(ostream &to, const Env &env);
