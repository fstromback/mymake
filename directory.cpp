#include "directory.h"

using namespace std;

Directory::Directory(string path) {
  initialize(path);
}

Directory::Directory(const File &f) {
  initialize(f.toString());
}

Directory::~Directory() {
}

#ifdef _WIN32

void Directory::initialize(string path) {
  File f(path);
  if (f.verifyDir()) {
    if (path[path.size() - 1] == PATH_DELIM) path = path.substr(0, path.size() - 1);

    this->path = File(path);

    WIN32_FIND_DATA file;
    HANDLE h = FindFirstFile((path + PATH_DELIM + string("*")).c_str(), &file);

    File directory = File(path);

    while (h) {
      File f = directory + file.cFileName;
      if (f.verifyDir()) folders.push_back(f);
      else files.push_back(f);
    }
    if (!h) FindClose(h);
  } else if (f.exists()) {
    this->path = f.parent();
    files.push_back(f);
  } else {
    cout << "Warning: The file " << path << " does not exist." << endl;
  }
}

#else
void Directory::initialize(string path) {
  File f(path);
  if (f.verifyDir()) {

    if (path[path.size() - 1] == PATH_DELIM) path = path.substr(0, path.size() - 1);
    
    this->path = f;
    
    struct dirent *de = NULL;
    DIR *d = opendir(path.c_str());
    if (d == NULL) return;

    File directory(path);

    while (de = readdir(d)) {
      File f = directory + de->d_name;
      if (f.verifyDir()) folders.push_back(f);
      else files.push_back(f);
    }

    closedir(d);
  } else if (f.exists()) {
    this->path = f.parent();
    files.push_back(f);
  } else {
    cout << "Warning: The file " << path << " does not exist." << endl;
  }
}
#endif

bool Directory::remove() const{
  bool success = true;
  for (list<File>::const_iterator i = folders.begin(); i != folders.end(); i++) {
    if (i->isPrevious()) {
    } else {
      success &= i->deleteFile();
    }
  }

  for (list<File>::const_iterator i = files.begin(); i != files.end(); i++) {
    success &= i->deleteFile();
  }

  return success;
}

void Directory::output(ostream &to) {
  to << "Content of " << path << ":\n";

  for (list<File>::iterator i = folders.begin(); i != folders.end(); i++) {
    File *f = &(*i);
    to << (*f);
    to << endl;
  }

  for (list<File>::iterator i = files.begin(); i != files.end(); i++) {
    File *f = &(*i);
    to << (*f);
    to << endl;
  }

}


ostream &operator <<(ostream &outputTo, Directory &output) {
  output.output(outputTo);
  return outputTo;
}
