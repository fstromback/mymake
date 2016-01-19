#pragma once
#include "path.h"

/**
 * System specific functions.
 */

class Env;

// Low-level exec functionality.
int exec(const Path &binary, const vector<String> &args, const Path &cwd, const Env *env);

// Run a command through a shell.
int shellExec(const String &command, const Path &cwd, const Env *env);

// Environment variables data type (system-specific).
typedef void *EnvData;

// Allocate a EnvData large enough for storing 'entries' entries, total length 'totalCount'.
EnvData allocEnv(nat entries, nat totalCount);

// Free EnvData.
void freeEnv(EnvData data);

// Get an entry from 'env'. Initialize 'pos' to 0. Returns 'false' if we're at the end.
bool readEnv(EnvData env, nat &pos, String &data);

// Write an entry to 'env'. Initialize 'pos' to 0.
void writeEnv(EnvData env, nat &pos, const String &data);

// Get all environment variables.
EnvData getEnv();

// Environment variable compairision function.
bool envLess(const char *a, const char *b);

// Separator for environment variables.
extern const char envSeparator;
