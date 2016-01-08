#pragma once
#include "path.h"

/**
 * System specific functions.
 */

int exec(const Path &binary, const vector<String> &args);

// Separator for environment variables.
extern const char envSeparator;

// Get/set environment variable.
String getEnv(const String &key);
void setEnv(const String &key, const String &value);
