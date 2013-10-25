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
    if (settings.debugOutput) cout << "Loaded includes for: " << title().c_str() << endl;
  } else {
    if (settings.debugOutput) cout << "Loaded cached includes for: " << title().c_str() << endl;
  }
}

void CppFile::loadIncludes() {
  if (settings.debugOutput) cout << "Generating includes for " << *this << "..." << endl;
  Files rootIncludes = Files::loadFromCpp(*this);
  
  includes.append(rootIncludes);

  for (list<File>::iterator i = includes.begin(); i != includes.end(); i++) {
    if (settings.debugOutput) cout << "Generating includes for " << i->title() << endl;
    Files now = Files::loadFromCpp(*i);
    includes.append(now);
  }

  if (settings.debugOutput) cout << "Saving result..." << endl;
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
