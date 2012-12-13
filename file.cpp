#include "file.h"
#include "mkpath.h"
#include "directory.h"
#include "globals.h"
#include "unistd.h"

#include <fstream>

using namespace std;

File::File(const string &path, const string &title) {
  initialize(path + title);
}

File::File(const string &f) {
  initialize(f);
}

void File::initialize(const string &f) {
  string filename = fixDelimiters(f);
  int lastPath = filename.rfind(PATH_DELIM);
  if (lastPath < 0) {
    this->directory = string(".") + PATH_DELIM;
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
  return *this;
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

void File::output(ostream &to) const {
  if (settings.debugOutput) {
    if (isDirectory()) {
      to << "[" << directory << ", " << title << "]";
    } else {
      to << directory << ", " << title;
    }
  } else {
    if (isDirectory()) {
      to << "[" << directory << title << "]";
    } else {
      to << directory << title;
    }
  }
}

ostream &operator <<(ostream &outputTo, const File &output) {
  output.output(outputTo);

  return outputTo;
}

ifstream *File::read() const {
  return new ifstream(getFullPath().c_str(), ifstream::in);
}

string File::trimPath(string path) {
  string dotSlash = string(".") + PATH_DELIM;
  int pos = path.find(dotSlash.c_str());
  while (pos >= 0) {
    if (pos == 0) {
      path = path.substr(pos + 2);
    } else if (path[pos - 1] != '.') {
      path = path.substr(0, pos) + path.substr(pos + 2);
    }
    pos = path.find(dotSlash.c_str(), pos + 1);
  }

  string slashDotDot = PATH_DELIM + string("..");
  pos = path.find(slashDotDot);
  while (pos >= 0) {
    int last = path.rfind(PATH_DELIM, pos - 1);
    if (last >= 0) {
      path = path.substr(0, last) + path.substr(pos + 3);
    } else {
      path = path.substr(pos + 4);
    }

    pos = path.find(slashDotDot, pos + 1);
  }

  return dotSlash + path;
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

string File::fixDelimiters(const string &path) {
	string result = path;
	for (int i = 0; i < result.size(); i++) {
		if (result[i] == '/') {
			result[i] = PATH_DELIM;
		} else if (result[i] == '\\') {
			result[i] = PATH_DELIM;
		}
	}
	return result;
}
