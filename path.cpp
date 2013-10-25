#include "path.h"
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


Path::Path(const string &path) {
  parseStr(path);
  simplify();
}

Path::Path() : isDirectory(true) {}

void Path::parseStr(const string &str) {
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

void Path::simplify() {
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

string Path::toString() const {
  string result = join(parts, string(1, PATH_DELIM));
  if (isDirectory && parts.size() > 0) result += PATH_DELIM;
  if (!isAbsolute()) result = string(".") + string(1, PATH_DELIM) + result;
  return result;

}

std::ostream &operator <<(std::ostream &to, const Path &path) {
  cout << path.toString();
  return to;
}


bool Path::operator ==(const Path &o) const {
  if (isDirectory != o.isDirectory) return false;
  if (parts.size() != o.parts.size()) return false;
  for (nat i = 0; i < parts.size(); i++) {
    if (parts[i] != o.parts[i]) return false;
  }
  return true;
}

Path Path::operator +(const Path &other) const {
  Path result(*this);
  result += other;
  return result;
}

Path &Path::operator +=(const Path &other) {
  isDirectory = other.isDirectory;
  parts.insert(parts.end(), other.parts.begin(), other.parts.end());
  simplify();
  return *this;
}

Path Path::operator +(const string &name) const {
  Path result(*this);
  result += name;
  return result;
}

Path &Path::operator +=(const string &name) {
  parts.push_back(name);
  isDirectory = false;
  return *this;
}

Path Path::parent() const {
  Path result(*this);
  result.parts.pop_back();
  result.isDirectory = true;
  return result;
}

string Path::title() const {
  return parts.back();
}

string Path::titleNoExt() const {
  string t = title();
  nat last = t.rfind('.');
  return t.substr(0, last);
}

bool Path::isDir() const {
  return isDirectory;
}

bool Path::isAbsolute() const {
  if (parts.size() == 0) return false;
  const string &first = parts.front();
#ifdef _WIN32
  if (first.size() < 2) return false;
  return first[1] == L':';
#else
  return (first == "");
#endif
}

bool Path::isEmpty() const {
  return parts.size() == 0;
}

bool Path::exists() const {
  struct stat status;
  if (stat(toString().c_str(), &status) < 0) return false;
  return true;
}

time_t Path::getLastModified() const {
  struct stat status;
  if (stat(toString().c_str(), &status) < 0) return 0;
  return status.st_mtime;
}

bool Path::verifyDir() const {
  struct stat status;
  if (stat(toString().c_str(), &status) < 0) return false;
  return S_ISDIR(status.st_mode);
}

Path Path::makeRelative(const Path &to) const {
  Path result;
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

Path Path::modifyRelative(const Path &old, const Path &n) const {
  Path rel = makeRelative(old);
  return n + rel;
}

bool Path::deleteFile() const {
  return (unlink(toString().c_str()) == 0);
}

void Path::setType(const string &type) {
  parts.back() = titleNoExt() + "." + type;
}

void Path::ensurePathExists() const {
  mkpath(toString().c_str(), 0764);
}

bool Path::isType(const string &ext) const {
  string last = parts.back();
  size_t dot = last.rfind('.');
  if (dot == string::npos) return false;

  return (last.substr(dot + 1) == ext);
}

bool Path::isPrevious() const {
  return title() == "." || title() == "..";
}

ifstream *Path::read() const {
  return new ifstream(toString().c_str());
}

void File::output(ostream &to) const {
  if (verifyDir()) {
    to << "[" << *this << "]";
  } else {
    to << *this;
  }
}


