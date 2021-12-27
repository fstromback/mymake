#include "std.h"
#include "commands.h"
#include "sync.h"

// Selected to be uncommon (or even illegal on Windows)
static const char SEPARATOR = ':';

Commands::Commands() {}

bool Commands::check(const String &file, const String &command) const {
	Lock::Guard z(lock);

	hash_map<String, String>::const_iterator found = files.find(file);

	// We consider it "same" if we don't have it in our storage.
	if (found == files.end())
		return true;

	return found->second == command;
}

void Commands::set(const String &file, const String &command) {
	Lock::Guard z(lock);

	files[file] = command;
}

void Commands::load(const Path &file) {
	Lock::Guard z(lock);

	ifstream src(toS(file).c_str());

	String line;
	while (getline(src, line)) {
		size_t pos = line.find(':');
		if (pos == String::npos)
			continue;

		files[line.substr(0, pos)] = line.substr(pos + 1);
	}
}

void Commands::save(const Path &file) const {
	Lock::Guard z(lock);

	ofstream dst(toS(file).c_str());

	// Keep ordering stable in the file.
	vector<std::pair<Path, String>> ordered(files.begin(), files.end());
	std::sort(ordered.begin(), ordered.end());

	for (size_t i = 0; i < ordered.size(); i++) {
		dst << ordered[i].first << SEPARATOR << ordered[i].second << '\n';
	}
}
