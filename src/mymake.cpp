#include "std.h"
#include "config.h"
#include "cmdline.h"
#include "compile.h"
#include "projectcompile.h"
#include "process.h"
#include "outputmgr.h"

// Compile a stand-alone .mymake-file.
int compileTarget(const Path &wd, const CmdLine &cmdline) {
	MakeConfig config;

	// Load the system-global file.
	Path globalFile(Path::home() + localConfig);
	if (globalFile.exists()) {
		DEBUG("Global file found: " << globalFile, VERBOSE);
		config.load(globalFile);
	}

	Path localFile(wd + localConfig);
	if (localFile.exists()) {
		config.load(localFile);
		DEBUG("Local file found: " << localFile, VERBOSE);
	}

	// Compile plain .mymake-file.
	Config params;
	config.apply(cmdline.names, params);
	cmdline.apply(config.options(), params);

	DEBUG("Configuration options: " << params, VERBOSE);

	// Set max # threads.
	ProcGroup::setLimit(to<nat>(params.getStr("maxThreads", "1")));

	compile::Target c(wd, params);
	if (cmdline.clean) {
		c.clean();
		return 0;
	}

	bool ok = c.find();

	if (ok)
		ok = c.compile();

	c.save();

	if (!ok) {
		PLN("Compilation failed!");
		return 1;
	}

	DEBUG("Compilation successful!", NORMAL);

	OutputMgr::shutdown();

	return c.execute(cmdline.params);
}

int compileProject(const Path &wd, const Path &projectFile, const CmdLine &cmdline) {
	MakeConfig config;

	Path globalFile(Path::home() + localConfig);
	if (globalFile.exists()) {
		DEBUG("Global file found: " << globalFile, VERBOSE);
		config.load(globalFile);
	}

	DEBUG("Project file found: " << projectFile, VERBOSE);
	config.load(projectFile);

	Config params;
	set<String> opts = cmdline.names;
	opts.insert("project");
	config.apply(opts, params);
	cmdline.apply(config.options(), params);

	DEBUG("Configuration options: " << params, VERBOSE);

	// Set max # threads.
	ProcGroup::setLimit(to<nat>(params.getStr("maxThreads", "1")));

	compile::Project c(wd, cmdline.names, config, params);
	DEBUG("-- Finding dependencies --", NORMAL);

	if (!c.find()) {
		c.save();
		PLN("Compilation failed!");
		return 1;
	}

	if (cmdline.clean) {
		c.clean();
		return 0;
	}

	bool ok = c.compile();
	c.save();

	if (!ok) {
		PLN("Compilation failed!");
		return 1;
	}

	DEBUG("-- Compilation successful! --", NORMAL);

	OutputMgr::shutdown();

	if (params.getBool("execute")) {
		return c.execute(cmdline.params);
	}

	return 0;
}

// Main entry-point for mymake.
int main(int argc, const char *argv[]) {
	CmdLine cmdline(vector<String>(argv, argv + argc));

#ifdef WINDOWS
	// Bash swallows crashes, which is very annoying. This makes it hard to attach the debugger when
	// using DebugBreak.
	SetErrorMode(0);
#endif

	if (cmdline.exit) {
		return 0;
	}

	if (cmdline.errors) {
		PLN("Errors in the command line!");
		cmdline.printHelp();
		return 1;
	}

	if (cmdline.showHelp) {
		cmdline.printHelp();
		return 0;
	}

	// Find a config file and cd there.
	Path newPath = findConfig();
	DEBUG("Working directory: " << newPath, INFO);

	// Load the local config-file.
	Path localProject(newPath + projectConfig);
	if (localProject.exists()) {
		return compileProject(newPath, localProject, cmdline);
	} else {
		return compileTarget(newPath, cmdline);
	}
}
