#include "std.h"
#include "cmdline.h"
#include "config.h"

namespace templ {
#include "../bin/templates.h"
}

static const pair<String, char> rawLongOptions[] = {
	make_pair("output", 'o'),
	make_pair("arguments", 'a'),
	make_pair("debug", 'd'),
	make_pair("force", 'f'),
	make_pair("execute", 'e'),
	make_pair("not", 'n'),
	make_pair("exec-path", 'p'),
	make_pair("project", '\3'),
	make_pair("target", '\2'),
	make_pair("config", '\1'),
	make_pair("help", '?'),
};
static const map<String, char> longOptions(rawLongOptions, rawLongOptions + ARRAY_COUNT(rawLongOptions));

static const char *helpStr =
	"Usage: %s [option...] [file...] [options]\n"
	"\n"
	"Tries to find a .mymake or .myproject file in the current directory or any\n"
	"parent directory and load any configuration there.\n"
	"\n"
	"Options:\n"
	"[option]        - use the corresponding section in the config file.\n"
	"[file]          - input files in addition to those specified in the config.\n"
	"--output, -o    - specify the name of the output file.\n"
	"--debug, -d     - specify debug level. 0 = silent, 3 = verbose.\n"
	"--exec-path, -p - specify the cwd when running the output.\n"
	"--help, -?      - show this help.\n"
	"--force, -f     - always recompile everything.\n"
	"--arguments, -a - arguments to the executed program. Must be last.\n"
	"--execute, -e   - execute the resulting file.\n"
	"--not, -n       - put in front of --execute (or use -ne) to not execute.\n"
	"--project       - generate a sample .myproject in cwd.\n"
	"--target        - generate a sample .mymake in cwd.\n"
	"--config        - write global config file.\n";

CmdLine::CmdLine(const vector<String> &params) :
	execute(tUnset),
	errors(false),
	showHelp(false),
	exit(false) {

	bool noOptions = false;
	state = sNone;

	execName = params[0];

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

void CmdLine::printHelp() const {
	printf(helpStr, execName.c_str());
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

static bool copyConfig() {
	ifstream src(toS(Path::home() + localConfig).c_str());
	if (!src)
		return false;

	ofstream dest(toS(Path::home() + localConfig).c_str());
	if (!dest)
		return false;

	String line;
	while (getline(src, line)) {
		if (line.empty())
			dest << line << endl;
		else if (line[0] = '#')
			dest << line << endl;
		else
			dest << '#' << line << endl;
	}

	return true;
}

static bool createGlobal() {
	ofstream dest(toS(Path::home() + localConfig).c_str());
	if (!dest) {
		WARNING("Failed to open " << Path::home() + localConfig);
		return false;
	}

	dest << templ::target;
	return true;
}

static bool createTarget() {
	ofstream dest(toS(Path::cwd() + localConfig).c_str());
	if (!dest) {
		WARNING("Failed to open " << Path::cwd() + localConfig);
		return false;
	}

	istringstream src(templ::target);

	String line;
	while (getline(src, line)) {
		if (line.empty()) {
			dest << endl;
		} else if (line[0] == '#' || line[0] == '[') {
			dest << line << endl;
		} else {
			dest << '#' << line << endl;
		}
	}

	return false;
}

static bool createProject() {
	ofstream dest(toS(Path::cwd() + projectConfig).c_str());
	if (!dest) {
		WARNING("Failed to open " << Path::cwd() + projectConfig);
		return false;
	}

	dest << templ::project;
	return false;
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
	case '?':
		showHelp = true;
		break;
	case '\1':
		if (!createGlobal())
			exit = errors = true;
		break;
	case '\2':
		if (!createTarget())
			exit = errors = true;
		break;
	case '\3':
		if (!createProject())
			exit = errors = true;
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
