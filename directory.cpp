#include "directory.h"

using namespace std;

Directory::Directory(string path) {
  initialize(path);
}

Directory::Directory(const File &f) {
  initialize(f.getFullPath());
}

Directory::~Directory() {
}

void Directory::initialize(string path) {
  File f(path);
  if (f.isDirectory()) {

    if (path[path.size() - 1] == '/') path = path.substr(0, path.size() - 1);
    
    this->path = path;
    
    struct dirent *de = NULL;
    DIR *d = opendir(path.c_str());
    if (d == NULL) return;

    string directory = path + "/";

    while (de = readdir(d)) {
      File f(directory, de->d_name);
      if (f.isDirectory()) folders.push_back(f);
      else files.push_back(f);
    }

    closedir(d);
  } else if (f.isValid()) {
    this->path = f.getDirectory();
    files.push_back(f);
  } else {
    cout << "Warning: The file " << path << " does not exist." << endl;
  }
}

bool Directory::remove() const{
  bool success = true;
  for (list<File>::const_iterator i = folders.begin(); i != folders.end(); i++) {
    if (i->getTitle() == ".") {
    } else if (i->getTitle() == "..") {
    } else {
      success &= i->remove();
    }
  }

  for (list<File>::const_iterator i = files.begin(); i != files.end(); i++) {
    success &= i->remove();
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
}
