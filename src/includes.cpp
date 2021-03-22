#include "std.h"
#include "includes.h"
#include "uniquequeue.h"

IncludeInfo::IncludeInfo() : ignored(false) {}

IncludeInfo::IncludeInfo(const Path &file, bool ignored) : file(file), ignored(ignored) {}

Timestamp IncludeInfo::lastModified() const {
	Timestamp r = file.mTime();
	for (hash_set<Path>::const_iterator i = includes.begin(); i != includes.end(); ++i)
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

Includes::Includes(const Path &wd, const vector<Path> &ip) : wd(wd), includePaths(ip) {}

Includes::Includes(const Path &wd, const Config &config) : wd(wd) {
	vector<String> paths = config.getArray("include");
	for (nat i = 0; i < paths.size(); i++) {
		includePaths << Path(paths[i]).makeAbsolute(wd);
	}
}

IncludeInfo Includes::info(const Path &file) {
	IncludeInfo result(file);

	// TODO? We may want to cache the result of this little search as well, but only until mymake
	// terminates, not more than that.
	UniqueQueue<Path> toExplore;
	toExplore << file;

	while (toExplore.any()) {
		Path f = toExplore.pop();
		const Info &at = fileInfo(f);

		// Update firstInclude if needed.
		if (result.firstInclude.empty())
			result.firstInclude = at.firstInclude;

		// See if it was ignored at some point.
		result.ignored |= at.ignored;

		// If ignored, stop traversing.
		if (at.ignored)
			continue;

		// Add recursively included headers.
		for (set<Path>::const_iterator i = at.includes.begin(), end = at.includes.end(); i != end; ++i) {
			toExplore << *i;
			result.includes << *i;
		}
	}

	return result;
}

const Includes::Info &Includes::fileInfo(const Path &file) {
	InfoMap::iterator i = cache.find(file);
	if (i == cache.end()) {
		return cache.insert(make_pair(file, createFileInfo(file))).first->second;
	} else {
		return i->second;
	}
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

Includes::Info Includes::createFileInfo(const Path &file) {
	Info r(file);

	// Ignored?
	if (ignored(file)) {
		r.ignored = true;
		return r;
	}

	ifstream in(toS(file).c_str());
	if (!in) {
		PLN(file << ":1: Failed to open file.");
		return r;
	}

	// Find includes.
	bool first = true;
	nat lineNr = 1;
	String include;
	String line;
	while (getline(in, line)) {
		if (isInclude(line, include)) {
			try {
				if (first)
					r.firstInclude = include;
				r.includes << resolveInclude(file, lineNr, include);
			} catch (const IncludeError &e) {
				PLN(e.what());
			}
			first = false;
		} else if (!isBlank(line)) {
			first = false;
		}

		lineNr++;
	}

	// We succeeded, mark it as valid.
	r.valid = true;
	return r;
}

Path Includes::resolveInclude(const Path &fromFile, nat lineNr, const String &src) const {
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
	message << " searched " << fromFile.parent();
	for (nat i = 0; i < includePaths.size(); i++) {
		message << endl << " searched " << includePaths[i];
	}

	throw IncludeError(message.str());
}

void Includes::load(const Path &from) {
	ifstream src(toS(from).c_str());
	String line;

	// Cache did not exist. No problem!
	if (!src)
		return;

	// Read and compare include paths.
	{
		nat incId = 0;
		while (getline(src, line)) {
			if (line.empty())
				continue;

			// End of includes?
			if (line[0] != 'i')
				break;

			if (incId >= includePaths.size())
				return;

			if (includePaths[incId++] != Path(line.substr(1)))
				return;
		}

		if (incId != includePaths.size())
			return;
	}

	// Include paths match, load the cache...

	Info *current = null;

	// The first line is already in 'line' since the previous loop reads one line too much.
	do {
		if (line.empty())
			continue;

		char type = line[0];
		String rest = line.substr(1);

		switch (type) {
		case '+': {
			// Done with the old one.
			current = null;

			nat space = rest.find(' ');
			if (space == String::npos)
				continue;

			Timestamp modified(to<nat64>(rest.substr(0, space)));
			Info file(Path(rest.substr(space + 1)));
			if (file.lastModified <= modified) {
				// Our cache is up to date!
				current = &cache.insert(make_pair(file.file, file)).first->second;
				current->valid = true;
			}

			break;
		}
		case '>':
			if (current)
				current->firstInclude = rest;
			break;
		case '-':
			if (current)
				current->includes.insert(Path(rest));
			break;
		}
	} while (getline(src, line));

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

		// Some file that did not exist, or was not accessible.
		if (!info.valid)
			continue;

		dest << "+" << info.lastModified.time << ' ' << info.file << endl;
		if (!info.firstInclude.empty())
			dest << ">" << info.firstInclude << endl;

		for (set<Path>::const_iterator i = info.includes.begin(); i != info.includes.end(); ++i) {
			dest << "-" << *i << endl;
		}
	}
}

void Includes::ignore(const vector<String> &patterns) {
	ignorePatterns = vector<Wildcard>(patterns.begin(), patterns.end());
}

bool Includes::ignored(const Path &path) const {
	String t = toS(path.makeRelative(wd));

	for (nat i = 0; i < ignorePatterns.size(); i++) {
		if (ignorePatterns[i].matches(t)) {
			DEBUG(path << " ignored as per " << ignorePatterns[i], INFO);
			return true;
		}
	}
	return false;
}

Includes::Info::Info() {}

Includes::Info::Info(const Path &file) : file(file), lastModified(file.mTime()), ignored(false), valid(false) {}

