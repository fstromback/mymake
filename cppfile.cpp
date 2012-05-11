#include "cppfile.h"

using namespace std;

CppFile::CppFile(string directory, string title) : File(directory, title) {
  updateIncludes();
}

CppFile::CppFile(const File &file) : File(file) {
  updateIncludes();
}

CppFile::~CppFile() {}

void CppFile::updateIncludes() {
  Files rootIncludes(*this);
  
  includes.append(rootIncludes);

  for (list<File>::iterator i = includes.begin(); i != includes.end(); i++) {
    Files now(*i);
    includes.append(now);
  }
}


void CppFile::output(ostream &to) {
  File::output(to);
  to << ":" << endl << includes;
}

time_t CppFile::getLastModified() const {
  time_t headers = includes.getLastModified();
  time_t me = File::getLastModified();
  return max(headers, me);
}
