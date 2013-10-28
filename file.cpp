#include "file.h"
#include "mkpath.h"
#include "unistd.h"
#include "directory.h"
#include "globals.h"
#include <algorithm>
#include <sstream>

typedef unsigned int nat;

string join(const std::vector<string> &data, const string &between) {
  if (data.size() == 0) return "";
  
  std::ostringstream oss;
  oss << data[0];
  for (nat i = 1; i < data.size(); i++) {
    oss << between << data[i];
  }
  return oss.str();
}


File::File(const string &path) {
  parseStr(path);
  simplify();
}

File::File() : isDirectory(true) {}

void File::parseStr(const string &str) {
  nat numPaths = std::count(str.begin(), str.end(), '\\');
  numPaths += std::count(str.begin(), str.end(), '/');
  parts.reserve(numPaths + 1);

  nat startAt = 0;
  for (nat i = 0; i < str.size(); i++) {
    if (str[i] == '\\' || str[i] == '/') {
      if (i > startAt) {
	parts.push_back(str.substr(startAt, i - startAt));
      }
      startAt = i + 1;
    }
  }

  if (str.size() > startAt) {
    parts.push_back(str.substr(startAt));
    isDirectory = false;
  } else {
    isDirectory = true;
  }
}

void File::simplify() {
  std::vector<string>::iterator i = parts.begin();
  while (i != parts.end()) {
    if (*i == ".") {
      // Safely removed.
      i = parts.erase(i);
    } else if (*i == ".." && i != parts.begin() && *(i - 1) != "..") {
      // Remove previous path.
      i = parts.erase(--i);
      i = parts.erase(i);
    } else {
      // Nothing to do, continue.
      ++i;
    }
  }
}

string File::toString() const {
  string result = join(parts, string(1, PATH_DELIM));
  if (isDirectory && parts.size() > 0) result += PATH_DELIM;
  if (!isAbsolute()) result = string(".") + string(1, PATH_DELIM) + result;
  return result;

}

std::ostream &operator <<(std::ostream &to, const File &path) {
  cout << path.toString();
  return to;
}


bool File::operator ==(const File &o) const {
  if (isDirectory != o.isDirectory) return false;
  if (parts.size() != o.parts.size()) return false;
  for (nat i = 0; i < parts.size(); i++) {
    if (parts[i] != o.parts[i]) return false;
  }
  return true;
}

File File::operator +(const File &other) const {
  File result(*this);
  result += other;
  return result;
}

File &File::operator +=(const File &other) {
  isDirectory = other.isDirectory;
  parts.insert(parts.end(), other.parts.begin(), other.parts.end());
  simplify();
  return *this;
}

File File::operator +(const string &name) const {
  File result(*this);
  result += name;
  return result;
}

File &File::operator +=(const string &name) {
  *this += File(name);
  return *this;
}

File File::parent() const {
  File result(*this);
  result.parts.pop_back();
  result.isDirectory = true;
  return result;
}

string File::title() const {
  return parts.back();
}

string File::titleNoExt() const {
  string t = title();
  nat last = t.rfind('.');
  return t.substr(0, last);
}

bool File::isDir() const {
  return isDirectory;
}

bool File::isAbsolute() const {
  if (parts.size() == 0) return false;
  const string &first = parts.front();
#ifdef _WIN32
  if (first.size() < 2) return false;
  return first[1] == L':';
#else
  return (first == "");
#endif
}

bool File::isEmpty() const {
  return parts.size() == 0;
}

bool File::exists() const {
  struct stat status;
  if (stat(toString().c_str(), &status) < 0) return false;
  return true;
}

time_t File::getLastModified() const {
  struct stat status;
  if (stat(toString().c_str(), &status) < 0) return 0;
  return status.st_mtime;
}

bool File::verifyDir() const {
  struct stat status;
  if (stat(toString().c_str(), &status) < 0) return false;
  return S_ISDIR(status.st_mode);
}

File File::makeRelative(const File &to) const {
  File result;
  result.isDirectory = isDirectory;

  bool equal = true;
  nat consumed = 0;

  for (nat i = 0; i < to.parts.size(); i++) {
    if (!equal) {
      result.parts.push_back("..");
    } else if (i >= parts.size()) {
      result.parts.push_back("..");
      equal = false;
    } else if (to.parts[i] != parts[i]) {
      result.parts.push_back("..");
      equal = false;
    } else {
      consumed++;
    }
  }

  for (nat i = consumed; i < parts.size(); i++) {
    result.parts.push_back(parts[i]);
  }

  return result;
}

File File::modifyRelative(const File &old, const File &n) const {
  File rel = makeRelative(old);
  return n + rel;
}

bool File::deleteFile() const {
  return (unlink(toString().c_str()) == 0);
}

void File::setType(const string &type) {
  if (type == "") {
    parts.back() = titleNoExt();
  } else {
    parts.back() = titleNoExt() + "." + type;
  }
}

void File::ensurePathExists() const {
  mkpath(parent().toString().c_str(), 0764);
}

bool File::isType(const string &ext) const {
  string last = parts.back();
  size_t dot = last.rfind('.');
  if (dot == string::npos) return false;

  return (last.substr(dot + 1) == ext);
}

bool File::isPrevious() const {
  return title() == "." || title() == "..";
}

ifstream *File::read() const {
  return new ifstream(toString().c_str());
}

void File::output(ostream &to) const {
  for (int i = 0; i < parts.size(); i++) to << ", " << parts[i];
  if (verifyDir()) {
    to << "[" << *this << "]";
  } else {
    to << *this;
  }
}

