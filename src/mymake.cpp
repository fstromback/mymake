#include "std.h"
#include "config.h"
#include "cmdline.h"
#include "compile.h"
#include "projectcompile.h"
#include "process.h"
#include "outputmgr.h"

// Load the global configuration file if it exists.
void loadGlobalConfig(const CmdLine &cmdline, MakeConfig &config) {
	Path globalFile(cmdline.globalConfig);
	if (globalFile.exists()) {
		DEBUG("Global configuration file found: " << globalFile, VERBOSE);
		config.load(globalFile);
	} else {
		WARNING("No global configuration file (" << globalFile << "). Use the --config flag to generate it.");
	}
}

// Compile a stand-alone .mymake-file.
int compileTarget(const Path &wd, const CmdLine &cmdline) {
	Timestamp start;
	MakeConfig config;

	// Load the system-global file.
	loadGlobalConfig(cmdline, config);

	Path localFile(wd + localConfig);
	if (localFile.exists()) {
		config.load(localFile);
		DEBUG("Local file found: " << localFile, VERBOSE);
	}

	// Compile plain .mymake-file.
	Config params;
	{
		String path = toS(wd);
		params.add("projectRoot", path);
		params.add("targetRoot", path);
	}
	config.apply(cmdline.names, params);
	cmdline.apply(config.options(), params);

	DEBUG("Configuration options: " << params, VERBOSE);

	params.env = Env::update(Env::current(), params);
	DEBUG("Environment variables for compilation: " << params.env, DEBUG);

	// Set max # threads.
	ProcGroup::setLimit(to<nat>(params.getStr("maxThreads", "1")));

	compile::Target c(wd, params);
	if (cmdline.clean) {
		c.clean();
		return 0;
	}

	Timestamp depStart;
	bool ok = c.find();
	Timestamp depEnd;
	if (cmdline.times)
		PLN("Found dependencies in " << (depEnd - depStart));

	if (ok) {
		Timestamp compStart;
		ok = c.compile();
		Timestamp compEnd;
		if (cmdline.times) {
			PLN("Compilation time: " << (compEnd - compStart));
			PLN("Total time: " << (compEnd - start));
		}
	}

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
	Timestamp start;
	MakeConfig config;

	loadGlobalConfig(cmdline, config);

	DEBUG("Project file found: " << projectFile, VERBOSE);
	config.load(projectFile);

	Config params;
	params.env = Env::current();
	params.add("projectRoot", toS(wd));

	set<String> opts = cmdline.names;
	opts.insert("project");
	config.apply(opts, params);
	cmdline.apply(config.options(), params);

	DEBUG("Configuration options: " << params, VERBOSE);

	// Set max # threads.
	ProcGroup::setLimit(to<nat>(params.getStr("maxThreads", "1")));

	compile::Project c(wd, cmdline.names, config, params, cmdline.times);
	DEBUG("-- Finding dependencies --", NORMAL);

	Timestamp depStart;
	bool ok = c.find();
	Timestamp depEnd;
	if (cmdline.times)
		PLN("Total time: " << (depEnd - depStart));

	if (!ok) {
		c.save();
		PLN("Compilation failed!");
		return 1;
	}

	if (cmdline.clean) {
		c.clean();
		return 0;
	}

	Timestamp compStart;
	ok = c.compile();
	Timestamp compEnd;
	c.save();

	if (!ok) {
		PLN("Compilation failed!");
		return 1;
	}

	if (cmdline.times) {
		PLN("Compilation time: " << (compEnd - compStart));
		PLN("Total time: " << (compEnd - start));
	}
	DEBUG("-- Compilation successful! --", NORMAL);

	OutputMgr::shutdown();

	if (params.getBool("execute")) {
		return c.execute(cmdline.params);
	}

	return 0;
}

// Main entry-point for mymake.
int real_main(int argc, const char *argv[]) {
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

// Wrapper around main to simplify management of the output state for the initial thread.
int main(int argc, const char *argv[]) {
	// Set up the initial output state.
	outputState = new OutputState();

	int result = real_main(argc, argv);

	outputState->unref();

	return result;
}
