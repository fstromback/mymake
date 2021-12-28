#pragma once
#include "path.h"
#include "hash.h"

/**
 * Information about the command-line used to compile various files.
 *
 * Note: Since this class might be used in a callback, it is thread-safe.
 */
class Commands {
public:
	// Create, specify input file.
	Commands();

	// Load data.
	void load(const Path &file);

	// Save the commands to file.
	void save(const Path &file) const;

	// Check if the command to compile a file is the same as the previous compilation. If no command
	// line was stored previously, we consider it to be the same.
	bool check(const String &file, const String &command);

	// Set the command to compile a given file.
	void set(const String &file, const String &command);

private:
	// Source file.
	Path file;

	// Lock for the 'files' map.
	mutable Lock lock;

	// Storage of all command lines. Paths are relative.
	hash_map<String, String> files;
};
