#include "std.h"
#include "includes.h"

IncludeInfo::IncludeInfo() : ignored(false) {}

IncludeInfo::IncludeInfo(const Path &file, bool ignored) : file(file), ignored(ignored) {}

Timestamp IncludeInfo::lastModified() const {
	Timestamp r = file.mTime();
	for (set<Path>::const_iterator i = includes.begin(); i != includes.end(); ++i)
		r = max(r, i->mTime());
	return r;
}

ostream &operator <<(ostream &to, const IncludeInfo &i) {
	to << i.file << ": ";
	if (!i.firstInclude.empty())
		to << "(first: " << i.firstInclude << ") ";
	join(to, i.includes);
	return to;
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

	// Ignored?
	if (ignored(file))
		return IncludeInfo(file, true);

	IncludeInfo r = recursiveIncludesIn(file);

	cache[file] = Info(r, Timestamp());

	return r;
}

Path Includes::resolveInclude(const Path &first, const Path &fromFile, nat lineNr, const String &src) const {
	Path sameFolder = fromFile.parent() + Path(src);
	if (sameFolder.exists())
		return sameFolder;

	for (nat i = 0; i < includePaths.size(); i++) {
		Path p = includePaths[i] + Path(src);
		if (p.exists())
			return p;
	}

	ostringstream message;
	message << fromFile << ":" << lineNr << ": The include " << src << " was not found!" << endl;
	message << " while finding includes from " << first << endl;
	message << " searched " << fromFile.parent();
	for (nat i = 0; i < includePaths.size(); i++) {
		message << endl << " searched " << includePaths[i];
	}

	throw IncludeError(message.str());
}

IncludeInfo Includes::recursiveIncludesIn(const Path &file) {
	bool first = true;
	IncludeInfo result(file);

	PathQueue to;
	to.push(file);

	while (to.any()) {
		Path now = to.pop();
		result.includes << now;

		if (first) {
			includesIn(file, now, to, &result.firstInclude);
			first = false;
		} else {
			includesIn(file, now, to, null);
		}
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

static bool isBlank(const String &line) {
	for (nat i = 0; i < line.size(); i++) {
		switch (line[i]) {
		case ' ':
		case '\t':
		case '\r':
			break;
		default:
			return false;
		}
	}
	return true;
}

void Includes::includesIn(const Path &firstFile, const Path &file, PathQueue &to, String *firstInclude) {
	ifstream in(toS(file).c_str());
	bool first = true;

	nat lineNr = 1;
	String include;
	String line;
	while (getline(in, line)) {
		if (isInclude(line, include)) {
			try {
				if (first && firstInclude)
					*firstInclude = include;
				to << resolveInclude(firstFile, file, lineNr, include);
			} catch (const IncludeError &e) {
				PLN(e.what());
			}
			first = false;
		} else if (!isBlank(line)) {
			first = false;
		}

		lineNr++;
	}
}

static Path readPath(istream &from) {
	String p;
	getline(from, p);
	return Path(p);
}

void Includes::load(const Path &from) {
	ifstream src(toS(from).c_str());
	if (!src) {
		PLN(from << ":1: Failed to open file.");
		return;
	}

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
		} else if (type == '>') {
			if (!current)
				return;

			getline(src, current->info.firstInclude);
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
		dest << ">" << info.info.firstInclude << endl;

		for (set<Path>::const_iterator i = info.info.includes.begin(); i != info.info.includes.end(); ++i) {
			dest << "-" << *i << endl;
		}
	}
}

void Includes::ignore(const vector<String> &patterns) {
	ignorePatterns = vector<Wildcard>(patterns.begin(), patterns.end());
}

bool Includes::ignored(const Path &path) const {
	for (nat i = 0; i < ignorePatterns.size(); i++) {
		if (ignorePatterns[i].matches(path.title())) {
			DEBUG(path << " ignored as per " << ignorePatterns[i], VERBOSE);
			return true;
		}
	}
	return false;
}
