#include <fstream>
#include <iostream>

#include "includecache.h"
#include "settings.h"
#include "globals.h"
#include "file.h"

IncludeCache::IncludeCache() {}

IncludeCache::~IncludeCache() {}

bool IncludeCache::load() {
  ifstream file(File(settings.getBuildPath(), "includes").getFullPath().c_str());
  if (!file) return true; //Filen finns inte, men detta räknas inte som ett fel!
  
  char firstChar;
  file >> firstChar;
  if (firstChar == 'E') return true; //End of file!
  if (firstChar != '+') return false;

  if (!file) return false;

  try {
    bool atEnd = false;
    while (!file.eof() && !atEnd) {
      CachedCpp cfile(file, atEnd);
      files[cfile.file] = cfile;
    }
  } catch (FileError &e) {
    files.clear();
    return false;
  }
  return true;
}

void IncludeCache::save() const {
  ofstream file(File(settings.getBuildPath(), "includes").getFullPath().c_str());

  for (map<string, CachedCpp>::const_iterator i = files.begin(); i != files.end(); i++) {
    i->second.write(file);
  }

  file << "E" << endl;
}


/////////////////////////////////////
// CachedFile
/////////////////////////////////////

IncludeCache::CachedFile::CachedFile() {}

IncludeCache::CachedFile::CachedFile(ifstream &from, bool &atEnd) {
  from >> modified;
  if (from.get() != ' ') throw FileError();
  getline(from, file);

  if (from.fail()) throw FileError();
}

IncludeCache::CachedFile::CachedFile(const string &file, time_t modified) {
  this->file = file;
  this->modified = modified;
}

IncludeCache::CachedFile::~CachedFile() {}

void IncludeCache::CachedFile::write(ofstream &to) const {
  to << "-" << modified << " " << file.c_str() << endl;
}

/////////////////////////////////////
// CachedCpp
/////////////////////////////////////

IncludeCache::CachedCpp::CachedCpp() {}

IncludeCache::CachedCpp::CachedCpp(ifstream &from, bool &atEnd) : CachedFile(from, atEnd) {
  char type;
  do {
    from >> type;
    switch (type) {
    case '-':
      includedFiles.push_back(CachedFile(from, atEnd));
      break;
    case 'E':
      atEnd = true;
    case '+':
      break;
    default:
      throw FileError();
      break;
    }
  } while (type == '-');
}

IncludeCache::CachedCpp::~CachedCpp() {}

void IncludeCache::CachedCpp::write(ofstream &to) const {
  to << "+" << modified << " " << file.c_str() << endl;

  for (list<CachedFile>::const_iterator i = includedFiles.begin(); i != includedFiles.end(); i++) {
    i->write(to);
  }

}

/////////////////////////////////////
// FileError
/////////////////////////////////////

IncludeCache::FileError::FileError() {}
