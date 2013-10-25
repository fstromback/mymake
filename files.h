#pragma once

#include <fstream>
#include <vector>
#include <string>
#include "file.h"
#include "directory.h"

using namespace std;

class IncludeCache;

class Files {
 public:
  Files(); //Create empty list of  files
  Files(const Directory &folder, string type);
  ~Files();

  friend ostream &operator <<(ostream &outputTo, const Files &output);

  inline list<File>::iterator begin() { return files.begin(); };
  inline list<File>::iterator end() { return files.end(); };
  inline size_t size() { return files.size(); };
  time_t getLastModified() const;

  Files changeFiletypes(const string &to);
  void append(const Files &files);
  void add(const File &file);

  bool load(const File &file);
  void save(const File &file) const;

  static Files loadFromCpp(const File &file);
 protected:

  list<File> files;

  void addFiles(const Directory &folder, string type);
  bool addCppHeader(const File &f);
  bool addCppHeader(const string &name);
  virtual void output(ostream &to) const;

  static vector<string> parseLine(const string &line);
};

