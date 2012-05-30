#include "file.h"
#include "mkpath.h"
#include "directory.h"
#include "globals.h"

#include <fstream>
#include <unistd.h>

using namespace std;

File::File(const string &path, const string &title) {
  this->directory = trimPath(path);
  this->title = title;
  if (stat((path + title).c_str(), &status) < 0) valid = false;
  else valid = true;
}

File::File(const string &filename) {
  int lastPath = filename.rfind('/');
  if (lastPath < 0) {
    this->directory = "./";
    this->title = filename;
  } else {
    this->directory = trimPath(filename.substr(0, lastPath + 1));
    this->title = filename.substr(lastPath + 1);
  }

  if (stat((directory + title).c_str(), &status) < 0) valid = false;
  else valid = true;
}

File::File(const File &other) {
  *this = other;
  if (stat((directory + title).c_str(), &status) < 0) valid = false;
  else valid = true;
}

File::~File() {}

File &File::operator =(const File &other) {
  this->status = other.status;
  this->directory = other.directory;
  this->title = other.title;
}

bool File::operator ==(const File &other) const {
  return (directory == other.directory) && (title == other.title);
}

bool File::isDirectory() const {
  return S_ISDIR(status.st_mode);
}

bool File::isPrevious() const {
  if (title == "..") return true;
  if (title == ".") return true;
  return false;
}

bool File::isType(const std::string &type) const {
  return (title.substr(title.size() - type.size()) == type);
}

string File::getTitle() const {
  return title;
}

string File::getFullPath() const {
  return directory + title;
}

string File::getDirectory() const {
  if (!isDirectory()) return directory;
  else return directory + title;
}

void File::output(ostream &to) {
  if (isDirectory()) {
    to << "[" << directory << title << "]";
  } else {
    to << directory << title;
  }
}

ostream &operator <<(ostream &outputTo, File &output) {
  output.output(outputTo);

  return outputTo;
}

ifstream *File::read() const {
  return new ifstream((directory + title).c_str());
}

string File::trimPath(string path) const {
  int pos = path.find("./");
  while (pos >= 0) {
    if (pos == 0) {
      path = path.substr(pos + 2);
    } else if (path[pos - 1] != '.') {
      path = path.substr(0, pos) + path.substr(pos + 2);
    }
    pos = path.find("./", pos + 1);
  }

  pos = path.find("/..");
  while (pos >= 0) {
    int last = path.rfind("/", pos - 1);
    if (last >= 0) {
      path = path.substr(0, last) + path.substr(pos + 3);
    } else {
      path = path.substr(pos + 4);
    }

    pos = path.find("/..", pos + 1);
  }

  return "./" + path;
}


time_t File::getLastModified() const {
  return status.st_mtime;
}


File File::modifyType(const string &type) const {
  string dottedType;
  if (type.size() == 0) dottedType = "";
  else dottedType = "." + type;

  int lastDot = title.rfind('.');
  if (lastDot >= 0) {
    return File(directory, title.substr(0, lastDot) + dottedType);
  } else {
    return File(directory, title + dottedType);
  }
}

File File::modifyRelative(const string &original, const string &newRelative) const {
  return File(newRelative + directory.substr(trimPath(original).size()), title);
}


void File::ensurePathExists() const {
  mkpath(directory.c_str(), 0764);
}

void File::update() {
  if (stat((directory + title).c_str(), &status) < 0) valid = false;
  else valid = true;
}

bool File::remove() const {
  if (isDirectory()) {
    Directory dir(*this);
    if (settings.debugOutput) cout << "Removing " << getFullPath().c_str() << endl;
    bool ok = dir.remove();
    ok &= (rmdir(getFullPath().c_str()) == 0);
    if (!ok) cout << "Failed to remove " << getFullPath().c_str() << endl;
    return ok;
  } else {
    if (settings.debugOutput) cout << "Removing " << getFullPath().c_str() << endl;
    bool ok = (unlink(getFullPath().c_str()) == 0);
    if (!ok) cout << "Failed to remove " << getFullPath().c_str() << endl;
    return ok;
  }
}
