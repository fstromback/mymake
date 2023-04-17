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
	// Create. Add some dummy components.
	Config();

	// Set a variable.
	void set(const String &key, const String &value);

	// Add to a variable.
	void add(const String &key, const String &value);
	void add(const String &key, const vector<String> &value);

	// Remove a variable.
	void clear(const String &key);


	// Has variable?
	bool has(const String &key) const;

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
	vector<String> replacement(String var, const map<String, String> &special) const;

	// Apply an operation to a string.
	vector<String> applyFn(const String &op, const vector<String> &original) const;
	pair<bool, String> applyFn(const String &op, const String &original) const;

	// Add prefix to each element.
	vector<String> addStr(const String &prefix, vector<String> to) const;
};


/**
 * Mymake-file. Stores the raw file contents without evaluating which variable assignments to apply.
 */
class MakeConfig {
public:
	// Null-config.
	MakeConfig();

	// Load file.
	bool load(const Path &file);

	// Apply to a config.
	void apply(set<String> config, Config &to) const;

	// Get all known options.
	const set<String> &options() const;

private:
	// Mode for assignment.
	enum Mode {
		mAppend,
		mSet,
	};

	// Assignment in a config file.
	struct Assignment {
		// Append?
		Mode mode;

		// Key, value;
		String key, value;
	};

	// Data in this config file.
	struct Section {
		// Valid for these options.
		set<String> options;

		// Valid when these options are excluded.
		set<String> exclude;

		// Assignments in this configuration.
		vector<Assignment> assignments;
	};

	// All data.
	vector<Section> sections;

	// All options.
	set<String> allOptions;

	// Parse section.
	void parseSection(String line);

	// Parse assignment.
	void parseAssignment(String line, nat initialSize);

	// Output:
	friend ostream &operator <<(ostream &to, const MakeConfig &c);
};

