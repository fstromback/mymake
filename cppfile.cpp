#include "cppfile.h"

#include "includecache.h"
#include "globals.h"

using namespace std;

CppFile::CppFile(string directory, string title) : File(directory, title) {
  updateIncludes();
}

CppFile::CppFile(const File &file) : File(file) {
  updateIncludes();
}

CppFile::~CppFile() {}

void CppFile::updateIncludes() {
  if (!includes.load(getFullPath())) {
    loadIncludes();
    if (settings.debugOutput) cout << "Loaded includes for: " << getTitle().c_str() << endl;
  } else {
    if (settings.debugOutput) cout << "Loaded cached includes for: " << getTitle().c_str() << endl;
  }
}

void CppFile::loadIncludes() {
  Files rootIncludes = Files::loadFromCpp(*this);
  
  includes.append(rootIncludes);

  for (list<File>::iterator i = includes.begin(); i != includes.end(); i++) {
    Files now = Files::loadFromCpp(*i);
    includes.append(now);
  }

  includes.save(getFullPath());
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
