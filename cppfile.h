#pragma once

#include "file.h"
#include "files.h"

using namespace std;

class CppFile : public File {
 public:
  CppFile(string directory, string title);
  CppFile(const File &file);
  virtual ~CppFile();

  virtual time_t getLastModified() const;
  Files &getIncludes() { return includes; };

  friend class Files;
 protected:
  Files includes;

  virtual void output(ostream &to);
  void updateIncludes();
  void loadIncludes();
};

