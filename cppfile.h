#ifndef CPPFILE_H
#define CPPFILE_H

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
 protected:
  Files includes;

  virtual void output(ostream &to);
  void updateIncludes();
};

#endif
