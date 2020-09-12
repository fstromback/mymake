#pragma once

#include "std.h"

/**
 * Captured environment variables.
 */
typedef map<string, vector<string>> EnvVars;

// Parse environment variables from executing "set" in "cmd.exe".
EnvVars parseEnv(istream &from);

// Capture the environment variables from CMD.exe after running some command.
EnvVars captureEnv(const string &command);

// Subtract all parts from one set from another.
EnvVars subtract(const EnvVars &full, const EnvVars &remove);

// Print them.
ostream &operator <<(ostream &to, const vector<string> &str);
ostream &operator <<(ostream &to, const EnvVars &vars);

