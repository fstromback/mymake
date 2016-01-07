#include "std.h"
#include "includes.h"

Timestamp IncludeInfo::lastModified() const {
	Timestamp r = file.mTime();
	for (set<Path>::const_iterator i = includes.begin(); i != includes.end(); ++i)
		r = max(r, i->mTime());
	return r;
}

Includes::Includes(const vector<Path> &ip) : includePaths(ip) {}

Includes::Includes(const Path &wd, const Config &config) {
	vector<String> paths = config.getArray("include");
	for (nat i = 0; i < paths.size(); i++) {
		includePaths << Path(paths[i]).makeAbsolute(wd);
	}
}

IncludeInfo Includes::info(const Path &file) {
	recursiveIncludesIn(file);
	return IncludeInfo();
}

Path Includes::resolveInclude(const Path &fromFile, const String &src) const {
	Path sameFolder = fromFile.parent() + Path(src);
	if (sameFolder.exists())
		return sameFolder;

	for (nat i = 0; i < includePaths.size(); i++) {
		Path p = includePaths[i] + Path(src);
		if (p.exists())
			return p;
	}

	throw IncludeError("The include " + src + " in file " + toS(fromFile) + " was not found!");
}

set<Path> Includes::recursiveIncludesIn(const Path &file) {
	set<Path> result;

	PathQueue to;
	to.push(file);

	while (to.any()) {
		Path now = to.pop();
		result << now;

		includesIn(now, to);
	}

	return result;
}

static bool isInclude(const String &line, String &out) {
	if (line.empty())
		return false;

	if (line[0] != '#')
		return false;

	nat inc = line.find("include");
	if (inc == String::npos)
		return false;

	nat startQuote = line.find('"', inc + 7);
	if (startQuote == String::npos)
		return false;

	nat endQuote = line.find('"', startQuote + 1);
	if (endQuote == String::npos)
		return false;

	out = line.substr(startQuote + 1, endQuote - startQuote - 1);
	return true;
}

void Includes::includesIn(const Path &file, PathQueue &to) {
	ifstream in(toS(file).c_str());

	String include;
	String line;
	while (getline(in, line)) {
		if (isInclude(line, include)) {
			try {
				to << resolveInclude(file, include);
			} catch (const IncludeError &e) {
				WARNING(e.what());
			}
		}
	}
}
