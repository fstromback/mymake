#pragma once
#include "path.h"

/**
 * Command line error.
 */
class CmdError : public Error {
public:
	CmdError(const String &msg) : msg(msg) {}
	const char *what() const { return msg.c_str(); }
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

	// Run the compiled file.
	Tristate execute;

	// Output file name.
	Path output;

	// Parameters to the executable.
	vector<String> params;

	// Files found.
	vector<Path> files;

	// Configuration options.
	set<String> options;

private:
	// What should the next param be?
	enum State {
		sNone,
		sNegate, // Next option is negated.
		sOutput,
		sArguments,
		sDebug,
	};

	State state;

	// Parse options.
	bool parseOptions(const String &opts);
	bool parseOption(char opt);

	// Add parameter to previous option.
	bool optionParam(const String &value);

	// Add a file of config.
	void addFile(const String &file);
};