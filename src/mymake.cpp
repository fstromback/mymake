#include "std.h"
#include "config.h"
#include "cmdline.h"
#include "compile.h"

// Main entry-point for mymake.
int main(int argc, const char *argv[]) {
	CmdLine cmdline(vector<String>(argv, argv + argc));

	if (cmdline.errors) {
		PLN("Errors in the command line...");
		TODO("Better error message!");
		return 1;
	}

	// Find a config file and cd there.
	Path newPath = findConfig();
	DEBUG("Working directory: " << newPath, INFO);

	MakeConfig config;

	// Load the system-global file.
	Path globalFile(Path::home() + localConfig);
	if (globalFile.exists()) {
		config.load(globalFile);
	}

	// Load the local config-file.
	Path localProject(newPath + projectConfig);
	if (localProject.exists()) {
		DEBUG("Project file found: " << localProject, INFO);
		return 0;
	}

	Path localFile(newPath + localConfig);
	if (localFile.exists()) {
		config.load(localFile);
		DEBUG("Local file found: " << localFile, INFO);
	}

	// Compile plain .mymake-file.
	Config params;
	config.apply(cmdline.options, params);
	cmdline.apply(params);

	DEBUG("Configuration options: " << params, VERBOSE);

	Compilation c(newPath, params);
	c.compile();

	return 0;
}
