#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <iostream>
#include <list>
#include <string>

#include "file.h"

using namespace std;

class Directory {
 public:

  Directory(string path);
  Directory(const File &f);
  ~Directory();

  friend ostream &operator <<(ostream &outputTo, Directory &output);

  list<File> files;
  list<File> folders;

  string getPath() const { return path; };

  bool remove() const;
 private:
  string path;

  void initialize(string path);
  void output(ostream &to);
};

#endif
