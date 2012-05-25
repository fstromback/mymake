#pragma once

#include "files.h"

#include <fstream>
#include <string>
#include <map>
#include <list>

using namespace std;

class IncludeCache {
public:
  IncludeCache();
  ~IncludeCache();

  bool load();
  void save() const;

  friend class Files;
private:
  class FileError {
  public:
    FileError();
  };

  class CachedFile {
  public:
    CachedFile();
    CachedFile(ifstream &from, bool &atEnd);
    CachedFile(const string &file, time_t modified);
    virtual ~CachedFile();
    
    string file;
    time_t modified;

    virtual void write(ofstream &to) const;
  };

  class CachedCpp : public CachedFile {
  public:
    CachedCpp();
    CachedCpp(ifstream &from, bool &atEnd);
    virtual ~CachedCpp();

    list<CachedFile> includedFiles;

    virtual void write(ofstream &to) const;
  };
    
  map<string, CachedCpp> files;
};

