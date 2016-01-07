#include "std.h"
#include "includes.h"

IncludeInfo::IncludeInfo() {}

IncludeInfo::IncludeInfo(const Path &file) : file(file) {}

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

Includes::Info::Info() {}

Includes::Info::Info(const IncludeInfo &info, const Timestamp &modified) : info(info), lastModified(modified) {}

Includes::Info::Info(const Path &file, const Timestamp &modified) : info(file), lastModified(modified) {}

bool Includes::Info::upToDate() const {
	return info.lastModified() <= lastModified;
}

IncludeInfo Includes::info(const Path &file) {
	InfoMap::iterator cached = cache.find(file);
	if (cached != cache.end()) {
		if (cached->second.upToDate()) {
			DEBUG("Serving " << file << " from cache!", VERBOSE);
			return cached->second.info;
		}
	}

	// We need to update the cache...
	IncludeInfo r(file);
	r.includes = recursiveIncludesIn(file);

	cache[file] = Info(r, Timestamp());

	return r;
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

static Path readPath(istream &from) {
	String p;
	getline(from, p);
	return Path(p);
}

void Includes::load(const Path &from) {
	ifstream src(toS(from).c_str());

	char type;

	// Compare include paths.
	nat incId = 0;
	while (src.peek() == 'i') {
		src.get();
		Path path = readPath(src);

		if (incId >= includePaths.size())
			return;

		if (includePaths[incId++] != path)
			return;
	}

	if (incId != includePaths.size())
		return;

	Info *current = null;

	while (src >> type) {
		if (type == '+') {
			Timestamp time;
			src >> time.time;
			src.get(); // Read space.

			Path path = readPath(src);

			cache[path] = Info(path, time);
			current = &cache[path];
		} else if (type == '-') {
			if (!current)
				return;

			current->info.includes << readPath(src);
		}
	}
}

void Includes::save(const Path &to) const {
	ofstream dest(toS(to).c_str());

	// Include paths, so that we can ignore loading if the include paths have changed.
	for (nat i = 0; i < includePaths.size(); i++) {
		dest << "i" << includePaths[i] << endl;
	}

	// All data.
	for (InfoMap::const_iterator i = cache.begin(); i != cache.end(); ++i) {
		const Info &info = i->second;

		dest << "+" << info.lastModified.time << ' ' << info.info.file << endl;

		for (set<Path>::const_iterator i = info.info.includes.begin(); i != info.info.includes.end(); ++i) {
			dest << "-" << *i << endl;
		}
	}
}
