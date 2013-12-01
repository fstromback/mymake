#pragma once

// Define some handy macros to generate debug output when desired. These
// depend on the settings to be properly initialized.

#include <iostream>

// Output a line (always). The left out parens are intentional to allow
// outputs like this: PLN("hello " << "world");
#define PLN(x) std::cout << x << std::endl

// Defines the different debug levels.
enum DebugLevel {
  NONE = 0, // No debug output.
  DEFAULT = 1, // Default output of compilation progress.
  SETTINGS = 2, // Rough summary of settings, shall not be considerably larger than no output.
  COMPILATION = 3, // Show the command lines run when compiling.
  INCLUDES = 4, // Show searching for includes.
  VERBOSE = 5, // Show output about internal details, such as cache.
};

// Output a line if the debug level is high enough.
#define DEBUG(level, x) if ((level) <= settings.debugLevel) PLN(x)

