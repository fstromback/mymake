#include "std.h"
#include "config.h"
#include "cmdline.h"
#include "compile.h"
#include "projectcompile.h"

// Compile a stand-alone .mymake-file.
int compileTarget(const Path &wd, const CmdLine &cmdline) {
	MakeConfig config;

	// Load the system-global file.
	Path globalFile(Path::home() + localConfig);
	if (globalFile.exists()) {
		DEBUG("Global file found: " << globalFile, INFO);
		config.load(globalFile);
	}

	Path localFile(wd + localConfig);
	if (localFile.exists()) {
		config.load(localFile);
		DEBUG("Local file found: " << localFile, INFO);
	}

	// Compile plain .mymake-file.
	Config params;
	config.apply(cmdline.options, params);
	cmdline.apply(params);

	DEBUG("Configuration options: " << params, VERBOSE);

	compile::Target c(wd, params);
	if (!c.find()) {
		PLN("Compilation failed!");
		return 1;
	}

	if (!c.compile()) {
		PLN("Compilation failed!");
		return 1;
	}

	DEBUG("Compilation successful!", NORMAL);

	if (params.getBool("execute")) {
		DEBUG("Running output: " << join(cmdline.params), INFO);
		return c.execute(cmdline.params);
	}

	return 0;
}

int compileProject(const Path &wd, const Path &projectFile, const CmdLine &cmdline) {
	DEBUG("Project file found: " << projectFile, INFO);
	MakeConfig config;
	config.load(projectFile);

	Config params;
	config.apply(cmdline.options, params);
	cmdline.apply(params);

	DEBUG("Configuration options: " << params, VERBOSE);

	compile::Project c(wd, config, params);
	if (!c.find()) {
		PLN("Compilation failed!");
		return 1;
	}

	if (!c.compile()) {
		PLN("Compilation failed!");
		return 1;
	}

	DEBUG("Compilation successful!", NORMAL);

	if (params.getBool("execute")) {
		DEBUG("Running output: " << join(cmdline.params), INFO);
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
	// SetErrorMode(0);
#endif

	if (cmdline.errors) {
		PLN("Errors in the command line...");
		TODO("Better error message!");
		return 1;
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
