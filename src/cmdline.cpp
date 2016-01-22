#include "std.h"
#include "cmdline.h"
#include "config.h"
#include <limits>

namespace templ {
#include "../bin/templates.h"
}

static const pair<String, char> rawLongOptions[] = {
	make_pair("output", 'o'),
	make_pair("arguments", 'a'),
	make_pair("debug", 'd'),
	make_pair("force", 'f'),
	make_pair("clean", 'c'),
	make_pair("execute", 'e'),
	make_pair("not", 'n'),
	make_pair("exec-path", 'p'),
	make_pair("project", '\3'),
	make_pair("target", '\2'),
	make_pair("config", '\1'),
	make_pair("default-input", '\4'),
	make_pair("help", '?'),
	make_pair("threads", 'j'),
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
	"--clean, -c     - clean the selected project/target. Deletes the entire execDir and buildDir!\n"
	"--arguments, -a - arguments to the executed program. Must be last.\n"
	"--execute, -e   - execute the resulting file.\n"
	"--not, -n       - put in front of --execute (or use -ne) to not execute.\n"
	"--project       - generate a sample .myproject in cwd.\n"
	"--target        - generate a sample .mymake in cwd.\n"
	"--config        - write global config file.\n"
	"--threads, -j   - compile in parallel if possible, using this many threads.\n"
	"--default-input - add this file as an input if no other is specified on command-line\n"
	"                  or in configuration. Useful when integrating with text editors.\n";

CmdLine::CmdLine(const vector<String> &params) :
	errors(false),
	exit(false),
	execute(tUnset),
	showHelp(false),
	clean(false),
	threads(0) {

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

static bool checkFile(const Path &file) {
	if (!file.exists())
		return true;

	char ans;
	do {
		std::cout << "File " << file << " already exists. Overwrite? (y/n) ";
		if (!(std::cin >> ans))
			return false;
	} while (ans != 'y' && ans != 'n');

	return ans == 'y';
}

static nat askThreads() {
	nat ans = 0;
	while (true) {
		std::cout << "Preferred number of threads to use during compilation? ";
		if (std::cin >> ans)
			return ans;

		std::cin.clear();
		std::cin.ignore(std::numeric_limits<size_t>::max());
	}
}

static bool createGlobal() {
	Path file = Path::home() + localConfig;
	if (!checkFile(file))
		return false;

	nat defThreads = askThreads();
	if (defThreads < 1)
		defThreads = 1;

	ofstream dest(toS(file).c_str());
	if (!dest) {
		WARNING("Failed to open " << Path::home() + localConfig);
		return false;
	}

	dest << templ::target;
	dest << endl;
	dest << "[]" << endl;
	dest << "#Default number of threads for compilation on this machine." << endl;
	dest << "maxThreads=" << defThreads << endl;
	return true;
}

static bool createTarget() {
	Path file = Path::cwd() + localConfig;
	if (!checkFile(file))
		return false;

	ofstream dest(toS(file).c_str());
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
	Path file = Path::cwd() + projectConfig;
	if (!checkFile(file))
		return false;

	ofstream dest(toS(file).c_str());
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
		execute = tYes;
		break;
	case 'e':
		execute = (state == sNegate) ? tNo : tYes;
		break;
	case 'd':
		state = sDebug;
		break;
	case 'c':
		clean = true;
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
	case 'j':
		state = sParallel;
		break;
	case '\1':
		if (!createGlobal())
			errors = true;
		exit = true;
		break;
	case '\2':
		if (!createTarget())
			errors = true;
		exit = true;
		break;
	case '\3':
		if (!createProject())
			errors = true;
		exit = true;
		break;
	case '\4':
		state = sDefaultInput;
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
	case sParallel:
		threads = to<int>(v);
		return true;
	case sDefaultInput:
		defaultInput = v;
		return true;
	default:
		return false;
	}
}

void CmdLine::addFile(const String &file) {
	names << file;
	order << file;
}

void CmdLine::apply(const set<String> &options, Config &config) const {
	if (!output.isEmpty())
		config.set("output", toS(output));

	if (!execPath.isEmpty())
		config.set("execPath", toS(execPath));

	for (nat i = 0; i < order.size(); i++) {
		if (options.count(order[i]) == 0) {
			Path file(order[i]);
			config.add("input", toS(file.makeAbsolute()));
		}
	}

	if (config.getArray("input").empty()) {
		config.add("input", toS(Path(defaultInput).makeAbsolute()));
	}

	if (execute == tYes)
		config.set("execute", "yes");
	if (execute == tNo)
		config.set("execute", "no");

	if (threads > 0) {
		config.set("maxThreads", toS(threads));
	}
}
