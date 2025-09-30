#include "std.h"
#include "path.h"
#include "system.h"
#include "sync.h"
#include "hash.h"


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

static void partHash(size_t &state, const String &part) {
	for (nat i = 0; i < part.size(); i++)
		state = ((state << 5) + state) + tolower(part[i]);
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

static void partHash(size_t &state, const String &part) {
	for (nat i = 0; i < part.size(); i++)
		state = ((state << 5) + state) + part[i];
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

Path Path::home() {
	char tmp[MAX_PATH + 1] = { 0 };
	SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, tmp);
	Path r(tmp);
	r.makeDir();
	return r;
}

Path Path::config() {
	char tmp[MAX_PATH + 1] = { 0 };
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, tmp);
	Path r(tmp);
	r += "mymake";
	r.makeDir();
	return r;
}

void Path::deleteFile() const {
	DeleteFile(toS(*this).c_str());
}

void Path::deleteDir() const {
	RemoveDirectory(toS(*this).c_str());
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
	vector<std::pair<String, bool>> pieces;

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

	// Note: technically not using the default comparator, but this is typically stricter than the
	// default anyway, so it is fine.
	std::sort(pieces.begin(), pieces.end());

	result.reserve(pieces.size());
	for (size_t i = 0; i < pieces.size(); i++) {
		result.push_back(*this + pieces[i].first);
		if (pieces[i].second)
			result.back().makeDir();
	}

	return result;
}

void Path::createDir() const {
	if (exists())
		return;

	if (isEmpty())
		return;

	parent().createDir();
	CreateDirectory(toS(*this).c_str(), NULL);
}

Timestamp fromFileTime(FILETIME ft);

FileInfo Path::info() const {
	FileInfo result(false);
	HANDLE hFile = CreateFile(toS(*this).c_str(),
							0, /* We only want metadata */
							FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, /* Allow access to others */
							NULL, /* Security attributes */
							OPEN_EXISTING, /* Action if not existing */
							FILE_ATTRIBUTE_NORMAL,
							NULL /* Template file */);
	if (hFile != INVALID_HANDLE_VALUE) {
		result.exists = true;
		FILETIME cTime, mTime;
		if (GetFileTime(hFile, &cTime, NULL, &mTime) == TRUE) {
			result.mTime = fromFileTime(mTime);
			result.cTime = fromFileTime(cTime);
		} else {
			// This is likely an implementation bug.
			WARNING("Failed to retrieve file times for file " << *this);
		}
		CloseHandle(hFile);
	}

	return result;
}

// We can do exists() cheaper than stat() on Windows.
bool Path::exists() const {
	return GetFileAttributes(toS(*this).c_str()) != INVALID_FILE_ATTRIBUTES;
}

#else

Path Path::cwd() {
	char tmp[512] = { 0 };
	if (!getcwd(tmp, 512))
		WARNING("Failed to get cwd!");
	Path r(tmp);
	r.makeDir();
	return r;
}

Path Path::home() {
	Path r(getenv("HOME"));
	r.makeDir();
	return r;
}

Path Path::config() {
	const char *xdgConfig = getenv("XDG_CONFIG_HOME");
	Path r;
	if (xdgConfig) {
		r = Path(xdgConfig);
	} else {
		r = home();
		r += ".config";
	}
	r += "mymake";
	r.makeDir();
	return r;
}

void Path::deleteFile() const {
	unlink(toS(*this).c_str());
}

void Path::deleteDir() const {
	rmdir(toS(*this).c_str());
}

bool Path::isAbsolute() const {
	if (parts.size() == 0)
		return false;
	return parts.front().empty();
}

vector<Path> Path::children() const {
	vector<Path> result;
	vector<std::pair<String, bool>> pieces;
	String prefix = toS(*this);

	DIR *h = opendir(prefix.c_str());
	if (h == null)
		return result;

	if (!prefix.empty() && prefix.back() != '/')
		prefix += '/';

	dirent *d;
	struct stat s;
	while ((d = readdir(h)) != null) {
		if (strcmp(d->d_name, "..") != 0 && strcmp(d->d_name, ".") != 0) {
			bool dir = false;

			switch (d->d_type) {
			case DT_UNKNOWN:
			case DT_LNK:
				if (stat((prefix + d->d_name).c_str(), &s) == 0) {
					if (S_ISDIR(s.st_mode))
						dir = true;
				} else {
					continue;
				}
				break;
			case DT_DIR:
				dir = true;
				break;
			}

			pieces.push_back(std::pair<String, bool>(d->d_name, dir));
		}
	}

	closedir(h);

	std::sort(pieces.begin(), pieces.end());

	result.reserve(pieces.size());
	for (size_t i = 0; i < pieces.size(); i++) {
		result.push_back(*this + pieces[i].first);
		if (pieces[i].second)
			result.back().makeDir();
	}

	return result;
}

void Path::createDir() const {
	if (exists())
		return;

	if (isEmpty())
		return;

	parent().createDir();
	mkdir(toS(*this).c_str(), 0777);
}

Timestamp fromFileTime(time_t);

FileInfo Path::info() const {
	FileInfo result(false);

	struct stat s;
	if (stat(toS(*this).c_str(), &s) == 0) {
		result.exists = true;
		result.mTime = fromFileTime(s.st_mtime);
		result.cTime = fromFileTime(s.st_ctime);
	}

	return result;
}

bool Path::exists() const {
	struct stat s;
	return stat(toS(*this).c_str(), &s) == 0;
}

#endif

Timestamp Path::mTime() const {
	return info().mTime;
}

Timestamp Path::cTime() const {
	return info().cTime;
}

bool Path::equal(const String &a, const String &b) {
	return partEq(a, b);
}

int Path::compare(const String &a, const String &b) {
	return partCmp(a, b);
}

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
	for (nat i = 0; i < parts.size(); i++) {
		if (o.parts.size() <= i)
			return false;

		int r = partCmp(parts[i], o.parts[i]);
		if (r != 0)
			return r < 0;
	}

	if (parts.size() < o.parts.size())
		return true;

	if (isDirectory != o.isDirectory)
		return o.isDirectory;

	return false;
}

bool Path::operator >(const Path &o) const {
	return o < *this;
}

size_t Path::hash() const {
	// djb2-inspired hash
	size_t r = 5381;
	for (nat i = 0; i < parts.size(); i++) {
		partHash(r, parts[i]);
		r = ((r << 5) + r) + '/'; // To differentiate between different divisions of slashes.
	}

	return r;
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

void Path::makeTitle(const String &str) {
	parts.back() = str;
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

void Path::recursiveDelete() const {
	if (!exists())
		return;

	if (isDir()) {
		vector<Path> sub = children();
		for (nat i = 0; i < sub.size(); i++) {
			sub[i].recursiveDelete();
		}

		deleteDir();
	} else {
		deleteFile();
	}
}
