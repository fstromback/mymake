#pragma once
#include "system.h"
#include "config.h"

/**
 * Storage of environment variables. Produces a system-specific data structure that can be sent
 * off to 'exec' or 'CreateProcess'.
 */
class Env : NoCopy {
public:
	// Create environment variables based on the current configuration, as well as whatever is
	// located inside the config.
	Env(const Config &config);

	// Destroy.
	~Env();

	// Get system-specific data.
	inline EnvData data() const { return d; }

private:
	// Data.
	EnvData d;
};
