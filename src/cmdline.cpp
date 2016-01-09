#include "std.h"
#include "cmdline.h"
#include "config.h"

static const pair<String, char> rawLongOptions[] = {
	make_pair("output", 'o'),
	make_pair("arguments", 'a'),
	make_pair("debug", 'd'),
	make_pair("force", 'f'),
	make_pair("exec-path", 'p'),
};
static const map<String, char> longOptions(rawLongOptions, rawLongOptions + ARRAY_COUNT(rawLongOptions));

CmdLine::CmdLine(const vector<String> &params) :
	execute(tUnset),
	errors(false) {

	bool noOptions = false;
	state = sNone;

	for (nat i = 1; i < params.size(); i++) {
		const String &c = params[i];

		if (optionParam(c)) {
			// Nothing.
		} else if (!noOptions && c.size() > 1 && c[0] == '-') {
			if (!parseOptions(c.substr(1))) {
				errors = true;
				break;
			}
		} else if (c == "--") {
			noOptions = true;
		} else {
			addFile(c);
		}
	}
}

bool CmdLine::parseOptions(const String &opts) {
	if (opts.size() > 1 && opts[0] == '-') {
		// Long option - only one.
		map<String, char>::const_iterator i = longOptions.find(opts.substr(1));
		if (i == longOptions.end())
			return false;

		return parseOption(i->second);
	} else {
		for (nat i = 0; i < opts.size(); i++) {
			if (!parseOption(opts[i]))
				return false;
		}
	}

	return true;
}

bool CmdLine::parseOption(char opt) {
	if (state != sNone && state != sNegate) {
		return false;
	}

	switch (opt) {
	case 'n':
		state = sNegate;
		break;
	case 'o':
		state = sOutput;
		break;
	case 'a':
		state = sArguments;
		break;
	case 'e':
		execute = (state == sNegate) ? tNo : tYes;
		break;
	case 'd':
		state = sDebug;
		break;
	case 'f':
		force = (state != sNegate);
		break;
	case 'p':
		state = sExecPath;
		break;
	default:
		return false;
	}

	return true;
}

bool CmdLine::optionParam(const String &v) {
	State s = state;
	state = sNone;

	switch (s) {
	case sOutput:
		output = Path(v);
		return true;
	case sArguments:
		params << v;
		state = sArguments;
		return true;
	case sDebug:
		debugLevel = to<int>(v);
		return true;
	case sExecPath:
		execPath = Path(v).makeAbsolute();
		return true;
	}

	return false;
}

void CmdLine::addFile(const String &file) {
	Path p(file);
	if (p.exists()) {
		files << p.makeAbsolute();
	} else {
		options.insert(file);
	}
}

void CmdLine::apply(Config &config) const {
	if (!output.isEmpty())
		config.set("output", toS(output));

	if (!execPath.isEmpty())
		config.set("execPath", toS(execPath));

	for (nat i = 0; i < files.size(); i++)
		config.add("input", toS(files[i]));

	if (execute == tYes)
		config.set("execute", "yes");
	if (execute == tNo)
		config.set("execute", "no");
}
