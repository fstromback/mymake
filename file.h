#pragma once

#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include "dirent.h"
#include <time.h>
#include <iostream>
#include <vector>

using namespace std;

// A class for managing path names.
class File {
  friend std::ostream &operator <<(std::ostream &to, const File &path);
public:
  // Create an empty path.
  File();

  // Get the current working directory.
  static File cwd();

  // Create a path object from a string.
  explicit File(const string &path);

  // Comparison.
  bool operator ==(const File &o) const;
  inline bool operator !=(const File &o) const { return !(*this == o); }

  // Concat this path with another path, the other path must be relative.
  File operator +(const File &other) const;
  File &operator +=(const File &other);

  // Add a string to go deeper into the hierarchy. If this object is not
  // a directory already, it will be made into one.
  File operator +(const string &file) const;
  File &operator +=(const string &file);

  // To string.
  string toString() const;

  // Status about this path.
  bool isDir() const;
  bool isAbsolute() const;
  bool isEmpty() const;

  // Make this obj a directory.
  void makeDir() { isDirectory = true; }

  // Get parent directory.
  File parent() const;

  // Get the title of this file or directory.
  string title() const;
  string titleNoExt() const;

  // Does the file exist?
  bool exists() const;

  // Delete the file.
  bool deleteFile() const;

  // Make this path relative to another path. Absolute-making is accomplished by
  // using the + operator above.
  File makeRelative(const File &to) const;

  // Make this path relative another path.
  File modifyRelative(const File &original, const File &newRel) const;

  void setType(const string &to);
  time_t getLastModified() const;

  void ensurePathExists() const;
  bool isType(const string &ext) const;
  bool verifyDir() const; // Check in the fs if the path is a dir.
  bool isPrevious() const;
  ifstream *read() const;

  // Output formatted to "out".
  void output(ostream &to) const;
private:

  // Internal representation is a list of strings, one for each part of the pathname.
  std::vector<string> parts;

  // Is this a directory?
  bool isDirectory;

  // Parse a path string.
  void parseStr(const string &str);

  // Simplify a path string, which means to remove any . and ..
  void simplify();
};
