#pragma once
#include "path.h"

/**
 * Configuration-file management.
 */

// Find a directory containing a suitable configuration file so we can cd there later.
Path findConfig();
