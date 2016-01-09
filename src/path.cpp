#include "std.h"
#include "path.h"
#include "system.h"

#ifdef WINDOWS

#include <direct.h>
#include <Shlwapi.h>
#include <Shlobj.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

static bool partEq(const String &a, const String &b) {
	return _stricmp(a.c_str(), b.c_str()) == 0;
}

static int partCmp(const String &a, const String &b) {
	return _stricmp(a.c_str(), b.c_str());
}

static const char separator = '\\';
static const char *separatorStr = "\\";

#else

#include <cstring>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

static bool partEq(const String &a, const String &b) {
	return a == b;
}

static int partCmp(const String &a, const String &b) {
	return strcmp(a.c_str(), b.c_str());
}

static const char separator = '/';
static const char *separatorStr = "/";

#endif

//////////////////////////////////////////////////////////////////////////
// The Path class
//////////////////////////////////////////////////////////////////////////

#ifdef WINDOWS

Path Path::cwd() {
	char tmp[MAX_PATH + 1];
	_getcwd(tmp, MAX_PATH + 1);
	Path r(tmp);
	r.makeDir();
	return r;
}

void Path::cd(const Path &to) {
	SetCurrentDirectory(toS(to).c_str());
}

Path Path::home() {
	char tmp[MAX_PATH + 1] = { 0 };
	SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, tmp);
	Path r(tmp);
	r.makeDir();
	return r;
}

bool Path::exists() const {
	return PathFileExists(toS(*this).c_str()) == TRUE;
}

void Path::deleteFile() const {
	DeleteFile(toS(*this).c_str());
}

bool Path::isAbsolute() const {
	if (parts.size() == 0)
		return false;
	const String &first = parts.front();
	if (first.size() < 2)
		return false;
	return first[1] == L':';
}

vector<Path> Path::children() const {
	vector<Path> result;

	String searchStr = toS(*this);
	if (!isDir())
		searchStr += "\\";
	searchStr += "*";

	WIN32_FIND_DATA findData;
	HANDLE h = FindFirstFile(searchStr.c_str(), &findData);
	if (h == INVALID_HANDLE_VALUE)
		return result;

	do {
		if (strcmp(findData.cFileName, "..") != 0 && strcmp(findData.cFileName, ".") != 0) {
			result.push_back(*this + findData.cFileName);
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				result.back().makeDir();
		}
	} while (FindNextFile(h, &findData));

	FindClose(h);

	return result;
}

Timestamp fromFileTime(FILETIME ft);

Timestamp Path::mTime() const {
	WIN32_FIND_DATA d;
	HANDLE h = FindFirstFile(toS(*this).c_str(), &d);
	if (h == INVALID_HANDLE_VALUE)
		return Timestamp(0);
	FindClose(h);

	return fromFileTime(d.ftLastWriteTime);
}

Timestamp Path::cTime() const {
	WIN32_FIND_DATA d;
	HANDLE h = FindFirstFile(toS(*this).c_str(), &d);
	if (h == INVALID_HANDLE_VALUE)
		return Timestamp(0);
	FindClose(h);

	return fromFileTime(d.ftCreationTime);
}

void Path::createDir() const {
	if (exists())
		return;

	if (isEmpty())
		return;

	parent().createDir();
	CreateDirectory(toS(*this).c_str(), NULL);
}


#else

Path Path::cwd() {
	char tmp[512];
	getcwd(tmp, 512);
	Path r(tmp);
	r.makeDir();
	return r;
}

void Path::cd(const Path &to) {
	chdir(toS(to).c_str());
}

Path Path::home() {
	Path r(getEnv("HOME"));
	r.makeDir();
	return r;
}

bool Path::exists() const {
	struct stat s;
	return stat(toS(*this).c_str(), &s) == 0;
}

void Path::deleteFile() const {
	unlink(toS(*this).c_str());
}

bool Path::isAbsolute() const {
	if (parts.size() == 0)
		return false;
	return parts.front().empty();
}

vector<Path> Path::children() const {
	vector<Path> result;

	DIR *h = opendir(toS(*this).c_str());
	if (h == null)
		return result;

	dirent *d;
	struct stat s;
	while ((d = readdir(h)) != null) {
		if (strcmp(d->d_name, "..") != 0 && strcmp(d->d_name, ".") != 0) {
			result.push_back(*this + d->d_name);

			if (stat(toS(result.back()).c_str(), &s) == 0) {
				if (S_ISDIR(s.st_mode))
					result.back().makeDir();
			} else {
				result.pop_back();
			}
		}
	}

	closedir(h);

	return result;
}

Timestamp fromFileTime(time_t);

Timestamp Path::mTime() const {
	struct stat s;
	if (stat(toS(*this).c_str(), &s))
		return Timestamp();

	return fromFileTime(s.st_mtime);
}

Timestamp Path::cTime() const {
	struct stat s;
	if (stat(toS(*this).c_str(), &s))
		return Timestamp();

	return fromFileTime(s.st_ctime);
}

void Path::createDir() const {
	if (exists())
		return;

	if (isEmpty())
		return;

	parent().createDir();
	mkdir(toS(*this).c_str(), 0777);
}

#endif

Path::Path(const String &path) : isDirectory(false) {
	parseStr(path);
	simplify();
}

Path::Path() : isDirectory(false) {}

void Path::parseStr(const String &str) {
	if (str.empty())
		return;

	nat numPaths = std::count(str.begin(), str.end(), '\\');
	numPaths += std::count(str.begin(), str.end(), '/');
	parts.reserve(numPaths + 1);

	if (str[0] == '\\' || str[0] == '/')
		parts.push_back("");

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
	vector<String>::iterator i = parts.begin();
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

std::ostream &operator <<(std::ostream &to, const Path &path) {
	join(to, path.parts, separatorStr);
	if (path.parts.empty())
		to << '.';
	if (path.isDir()) to << separatorStr;
	return to;
}

bool Path::operator ==(const Path &o) const {
	if (isDirectory != o.isDirectory)
		return false;
	if (parts.size() != o.parts.size())
		return false;

	for (nat i = 0; i < parts.size(); i++)
		if (!partEq(parts[i], o.parts[i]))
			return false;
	return true;
}

bool Path::operator <(const Path &o) const {
	if (isDirectory != o.isDirectory && parts == o.parts)
		return o.isDirectory;

	for (nat i = 0; i < parts.size(); i++) {
		int r = partCmp(parts[i], o.parts[i]);
		if (r != 0)
			return r < 0;
	}

	return false;
}

bool Path::operator >(const Path &o) const {
	return o < *this;
}


Path Path::operator +(const Path &other) const {
	Path result(*this);
	result += other;
	return result;
}

Path &Path::operator +=(const Path &other) {
	assert(!other.isAbsolute());
	isDirectory = other.isDirectory;
	parts.insert(parts.end(), other.parts.begin(), other.parts.end());
	simplify();
	return *this;
}

Path Path::operator +(const String &name) const {
	Path result(*this);
	result += name;
	return result;
}

Path &Path::operator +=(const String &name) {
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

String Path::first() const {
	return parts.front();
}

String Path::title() const {
	return parts.back();
}

String Path::titleNoExt() const {
	String t = title();
	nat last = t.rfind('.');
	return t.substr(0, last);
}

String Path::ext() const {
	String t = title();
	nat p = t.rfind('.');
	if (p == String::npos)
		return "";
	else
		return t.substr(p + 1);
}

void Path::makeExt(const String &str) {
	if (str.empty()) {
		parts.back() = titleNoExt();
	} else {
		parts.back() = titleNoExt() + "." + str;
	}
}

bool Path::isDir() const {
	return isDirectory;
}

bool Path::isEmpty() const {
	return parts.size() == 0;
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
		} else if (!partEq(to.parts[i], parts[i])) {
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

Path Path::makeAbsolute() const {
	return makeAbsolute(Path::cwd());
}

Path Path::makeAbsolute(const Path &to) const {
	if (isAbsolute())
		return *this;

	return to + *this;
}

bool Path::isChild(const Path &path) const {
	if (parts.size() <= path.parts.size())
		return false;

	for (nat i = 0; i < path.parts.size(); i++) {
		if (!partEq(parts[i], path.parts[i]))
			return false;
	}

	return true;
}
