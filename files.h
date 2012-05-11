#ifndef FILES_H
#define FILES_H

#include <fstream>
#include <vector>
#include <string>
#include "file.h"
#include "directory.h"

using namespace std;

class Files {
 public:
  Files(); //Create empty list of  files
  Files(const File &file); //Load include files from the current c/cpp file.
  Files(const Directory &folder, string type);
  ~Files();

  friend ostream &operator <<(ostream &outputTo, Files &output);

  inline list<File>::iterator begin() { return files.begin(); };
  inline list<File>::iterator end() { return files.end(); };
  inline size_t size() { return files.size(); };
  time_t getLastModified() const;

  Files changeFiletypes(const string &to);
  void append(const Files &files);
  void add(const File &file);
 protected:
  list<File> files;

  void addFiles(const Directory &folder, string type);
  virtual void output(ostream &to);

  vector<string> parseLine(const string &line);
};

#endif
