#include "std.h"
#include "config.h"
#include "cmdline.h"

// Main entry-point for mymake.
int main(int argc, const char *argv[]) {
	CmdLine params(vector<String>(argv, argv + argc));

	if (params.errors) {
		PLN("Errors in the command line...");
		TODO("Better error message!");
		return 1;
	}

	// Find a config file and cd there.
	Path newPath = findConfig();
	DEBUG("Working directory: " << newPath, INFO);
	Path::cd(newPath);

	return 0;
}
