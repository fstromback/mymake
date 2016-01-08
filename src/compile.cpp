#include "std.h"
#include "compile.h"
#include "wildcard.h"
#include "system.h"
#include "env.h"

namespace compile {

	Target::Target(const Path &wd, const Config &config) :
		wd(wd),
		config(config),
		includes(wd, config),
		buildDir(wd + Path(config.getStr("buildDir"))),
		validExts(config.getArray("ext")),
		compileVariants(config.getArray("compile")),
		intermediateExt(config.getStr("intermediateExt")) {

		// Create build directory if it is not already created.
		buildDir.createDir();

		// Load cached data if possible.
		if (!force) {
			includes.load(buildDir + "includes");
		}
	}

	Target::~Target() {
		includes.save(buildDir + "includes");
	}

	bool Target::find() {
		DEBUG("Compiling project in " << wd, INFO);
		toCompile.clear();

		PathQueue q;

		// TODO: Check pre-compiled header.
		// addFile(q, config.getStr("precompiledHeader"));

		// Add initial files.
		addFiles(q, config.getArray("input"));

		// Process files...
		while (q.any()) {
			Path now = q.pop();

			// Check if 'now' is inside the working directory.
			if (!now.isChild(wd)) {
				// Maybe a reference to another target from the same project?
				TODO("Keep track of depencies to other projects!");

				// No need to follow further!
				continue;
			}

			toCompile << now;

			// Add all other files we need.
			IncludeInfo info = includes.info(now);
			for (set<Path>::const_iterator i = info.includes.begin(); i != info.includes.end(); ++i) {
				addFile(q, *i);
			}
		}

		String outputName = config.getStr("output");
		if (outputName == "") {
			vector<String> input = config.getArray("input");
			if (input.empty()) {
				PLN("No input files.");
				return false;
			}
			outputName = input.front();
		}

		Path execDir = wd + Path(config.getStr("execDir"));
		execDir.createDir();

		output = execDir + Path(outputName).titleNoExt();
		output.makeExt(config.getStr("execExt"));

		return true;
	}

	bool Target::compile() {
		Path::cd(wd);
		DEBUG("CD into: " << wd, VERBOSE);

		Env env(config);

		map<String, String> data;
		data["file"] = "";
		data["output"] = "";

		Timestamp latestModified;
		ostringstream intermediateFiles;

		for (nat i = 0; i < toCompile.size(); i++) {
			const Path &src = toCompile[i];
			Path output = src.makeRelative(wd).makeAbsolute(buildDir);
			output.makeExt(intermediateExt);

			output.parent().createDir();

			String file = toS(src.makeRelative(wd));
			String out = toS(output.makeRelative(wd));
			if (i > 0)
				intermediateFiles << ' ';
			intermediateFiles << out;

			if (!force && output.exists() && output.mTime() >= src.mTime()) {
				DEBUG("Skipping " << src << "...", VERBOSE);
				continue;
			}

			DEBUG("Compiling " << src.makeRelative(wd) << "...", NORMAL);

			String cmd = chooseCompile(file);
			if (cmd == "") {
				PLN("No suitable compile command-line for " << file);
				return false;
			}

			data["file"] = file;
			data["output"] = out;
			cmd = config.expandVars(cmd, data);

			DEBUG("Command line: " << cmd, VERBOSE);
			if (system(cmd.c_str()) != 0) {
				// Abort compilation.
				return false;
			}

			Timestamp mTime = output.mTime();
			if (i == 0)
				latestModified = mTime;
			else
				latestModified = max(latestModified, mTime);

		}

		// Link the output.
		if (!force && output.exists() && output.mTime() >= latestModified) {
			DEBUG("Skipping linking step.", VERBOSE);
			return true;
		}

		data["files"] = intermediateFiles.str();
		data["output"] = toS(output.makeRelative(wd));
		String cmd = config.getVars("link", data);
		DEBUG("Command line: " << cmd, VERBOSE);

		if (system(cmd.c_str()) != 0) {
			return false;
		}

		return true;
	}

	int Target::execute(const vector<String> &params) const {
		Path::cd(wd);
		return exec(output.makeRelative(wd), params);
	}

	void Target::addFiles(PathQueue &to, const vector<String> &src) {
		for (nat i = 0; i < src.size(); i++) {
			addFile(to, src[i]);
		}
	}

	bool Target::findExt(Path &path) const {
		for (nat i = 0; i < validExts.size(); i++) {
			path.makeExt(validExts[i]);
			if (path.exists())
				return true;
		}

		return false;
	}

	void Target::addFile(PathQueue &to, const String &src) {
		Path path(src);
		path = path.makeAbsolute(wd);

		if (!path.exists()) {
			if (!findExt(path)) {
				WARNING("The file " << src << " does not exist.");
				return;
			}
		}

		to.push(path);
	}

	void Target::addFile(PathQueue &to, const Path &header) {
		Path impl = header;
		if (!findExt(impl)) {
			// No cpp file for a header... No big deal.
			return;
		}

		to.push(impl);
	}

	String Target::chooseCompile(const String &file) {
		for (nat i = compileVariants.size(); i > 0; i--) {
			const String &variant = compileVariants[i - 1];

			nat colon = variant.find(':');
			if (colon == String::npos) {
				WARNING("Compile variable without telling what filetypes it is for: " << variant);
				continue;
			}

			Wildcard w(variant.substr(0, colon));
			if (w.matches(file))
				return variant.substr(colon + 1);

			DEBUG("No match for " << file << " using " << variant, VERBOSE);
		}

		return "";
	}

}
