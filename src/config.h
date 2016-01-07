#pragma once
#include "path.h"

/**
 * Configuration-file management.
 */

extern const String localConfig;
extern const String projectConfig;


// Find a directory containing a suitable configuration file so we can cd there later.
Path findConfig();


/**
 * Merged configuration.
 */
class Config {
public:
	// Set a variable.
	void set(const String &str, const String &value);

	// Add to a variable.
	void add(const String &str, const String &value);

	// Get a variable as a string.
	String getStr(const String &key, const String &def = "") const;

	// Get a variable as an array.
	vector<String> getArray(const String &key, const vector<String> &def = vector<String>()) const;

	// Get as a boolean.
	bool getBool(const String &key, bool def = false) const;

	// Get with replaced variables.
	String getVars(const String &key, const map<String, String> &special = map<String, String>()) const;

	// Insert variables into a string.
	String expandVars(const String &into, const map<String, String> &special = map<String, String>()) const;

	// Output.
	friend ostream &operator <<(ostream &to, const Config &c);
private:
	typedef vector<String> Value;

	typedef map<String, Value> Data;
	Data data;

	// Get replacement for variable.
	String replacement(const String &var, const map<String, String> &special) const;

	// Get string for inserting a string in front of each element in another string.
	String buildString(const String &repKey, const String &arrayKey) const;
	String buildStringPath(const String &repKey, const String &arrayKey) const;
};

/**
 * Project file config.
 */
class ProjectConfig {
public:
	// TODO...
};

/**
 * Mymake-file.
 */
class MakeConfig {
public:
	// Null-config.
	MakeConfig();

	// Load file.
	bool load(const Path &file);

	// Apply to a config.
	void apply(const set<String> &config, Config &to) const;

private:
	// Assignment in a config file.
	struct Assignment {
		// Append?
		bool append;

		// Key, value;
		String key, value;
	};

	// Data in this config file.
	struct Section {
		// Valid for these configurations.
		set<String> configurations;

		// Assignments in this configuration.
		vector<Assignment> assignments;
	};

	// All data.
	vector<Section> sections;

	// Parse section.
	void parseSection(String line);

	// Parse assignment.
	void parseAssignment(String line);

	// Output:
	friend ostream &operator <<(ostream &to, const MakeConfig &c);
};

