#pragma once
#include "path.h"

class Config;

/**
 * Command line error.
 */
class CmdError : public Error {
public:
	CmdError(const String &msg) : msg(msg) {}
	inline ~CmdError() throw() {}
	const char *what() const throw() { return msg.c_str(); }
private:
	String msg;
};

/**
 * Command line parsing and storage.
 */
class CmdLine : NoCopy {
public:
	// Parse the command line.
	CmdLine(const vector<String> &params);

	// Any errors during parsing of the command line?
	bool errors;

	// Exit without doing anything.
	bool exit;

	// Run the compiled file.
	Tristate execute;

	// Output file name.
	Path output;

	// Parameters to the executable.
	vector<String> params;

	// Files or options.
	set<String> names;

	// Path to cd to before executing the final binary.
	Path execPath;

	// Show help?
	bool showHelp;

	// Clean project?
	bool clean;

	// Show times.
	bool times;

	// Apply to Config object.
	void apply(const set<String> &usedOptions, Config &config) const;

	// Print help.
	void printHelp() const;

private:
	// What should the next param be?
	enum State {
		sNone,
		sNegate, // Next option is negated.
		sOutput,
		sArguments,
		sDebug,
		sExecPath,
		sParallel,
		sDefaultInput,
		sCreateGlobal,
	};

	State state;

	// Name of the executable.
	String execName;

	// Order of the files/options.
	vector<String> order;

	// Add this file to input if none other exists. empty = do nothing.
	String defaultInput;

	// Number of threads to use. 0 = no opinion.
	nat threads;

	// Parameter to the global config option.
	String configParam;

	// Create global config?
	bool createGlobal;

	// Parse options.
	bool parseOptions(const String &opts);
	bool parseOption(char opt);

	// Add parameter to previous option.
	bool optionParam(const String &value);

	// Add a file of config.
	void addFile(const String &file);
};
