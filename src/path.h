#pragma once

#include "timestamp.h"

/**
 * A class for managing path names.
 */
class Path {
	friend std::ostream &operator <<(std::ostream &to, const Path &path);
public:
	// Current working directory.
	static Path cwd();

	// Set cwd.
	static void cd(const Path &to);

	// User's home directory.
	static Path home();

	// Create an empty path.
	Path();

	// Create a path object from a string.
	explicit Path(const String &path);

	// Comparison.
	bool operator ==(const Path &o) const;
	inline bool operator !=(const Path &o) const { return !(*this == o); }
	bool operator <(const Path &o) const;
	bool operator >(const Path &o) const;

	// Concat this path with another path, the other path must be relative.
	Path operator +(const Path &other) const;
	Path &operator +=(const Path &other);

	// Add a string to go deeper into the hierarchy. If this object is not
	// a directory already, it will be made into one.
	Path operator +(const String &file) const;
	Path &operator +=(const String &file);

	// Status about this path.
	bool isDir() const;
	bool isAbsolute() const;
	bool isEmpty() const;

	// Make this obj a directory.
	inline void makeDir() { isDirectory = true; }

	// Get parent directory.
	Path parent() const;

	// Get the title of this file or directory.
	String title() const;
	String titleNoExt() const;

	// Get file extension (always the last one).
	String ext() const;

	// Does the file exist?
	bool exists() const;

	// Delete the file.
	void deleteFile() const;

	// Make this path relative to another path. Absolute-making is accomplished by
	// using the + operator above.
	Path makeRelative(const Path &to) const;

	// Make this path absolute if it is not already.
	Path makeAbsolute() const;

	// Find the children of this path.
	vector<Path> children() const;

	// Modified time.
	Timestamp mTime() const;

	// Created time.
	Timestamp cTime() const;

private:

	// Internal representation is a list of strings, one for each part of the pathname.
	vector<String> parts;

	// Is this a directory?
	bool isDirectory;

	// Parse a path string.
	void parseStr(const String &str);

	// Simplify a path string, which means to remove any . and ..
	void simplify();
};