#include "cppfile.h"

#include "includecache.h"
#include "globals.h"

using namespace std;

// CppFile::CppFile(string directory, string title) : File(directory, title) {
//   updateIncludes();
// }

CppFile::CppFile(const File &file) : File(file) {
  updateIncludes();
}

CppFile::~CppFile() {}

void CppFile::updateIncludes() {
  if (!includes.load(*this)) {
    loadIncludes();
    DEBUG(VERBOSE, "Found includes for " << *this);
  } else {
    DEBUG(VERBOSE, "Loaded cached includes for " << *this);
  }
}

void CppFile::loadIncludes() {
  DEBUG(VERBOSE, "Generating includes for " << *this << "...");
  Files rootIncludes = Files::loadFromCpp(*this);
  
  includes.append(rootIncludes);

  for (list<File>::iterator i = includes.begin(); i != includes.end(); i++) {
    DEBUG(VERBOSE, "Generating includes for " << *this << " from " << *i << "...");
    Files now = Files::loadFromCpp(*i);
    includes.append(now);
  }

  DEBUG(VERBOSE, "Saving result...");
  includes.save(*this);
}

void CppFile::output(ostream &to) const {
  File::output(to);
  to << ":" << endl << includes;
}

time_t CppFile::getLastModified() const {
  time_t headers = includes.getLastModified();
  time_t me = File::getLastModified();
  return max(headers, me);
}
