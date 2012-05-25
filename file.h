#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#include <iostream>

using namespace std;

class File {
 public:
  File(const string &directory, const string &title);
  File(const string &filename);
  File(const File &file);
  virtual ~File();

  File &operator =(const File &other);
  bool operator ==(const File &other) const;
  friend ostream &operator <<(ostream &outputTo, File &output);

  string getTitle() const;
  string getFullPath() const;
  string getDirectory() const;
  bool isValid() const { return valid; };
  bool isDirectory() const;
  bool isPrevious() const;
  bool isType(const string &type) const;
  virtual time_t getLastModified() const;
  File modifyType(const string &type) const;
  File modifyRelative(const string &original, const string &newRelative) const;

  void ensurePathExists() const;

  void update();
  ifstream* read() const;
 protected:
  struct stat status;
  bool valid;

  string directory;
  string title;

  virtual void output(ostream &to);
  string trimPath(string path) const;
};


