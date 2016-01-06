#include "std.h"
#include "config.h"

static const String localConfig = ".mymake";
static const String projectConfig = ".myproject";

// Known bug: can not compile stuff in the root directory...
Path findConfig() {
	Path home = Path::home();

	for (Path dir = Path::cwd(); !dir.isEmpty(); dir = dir.parent()) {
		// Skip home directory, where the global configuration is located.
		if (dir == home)
			continue;

		// Search upwards for a configuration...
		if ((dir + projectConfig).exists())
			return dir;

		if ((dir + localConfig).exists()) {
			// Parent directory might contain a valid configuration!
			if ((dir.parent() + projectConfig).exists())
				return dir.parent();

			return dir;
		}
	}

	// Default to cwd.
	return Path::cwd();
}
