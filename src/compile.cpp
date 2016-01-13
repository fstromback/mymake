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
		validExts(config.getArray("ext")),
		compileVariants(config.getArray("compile")),
		buildDir(wd + Path(config.getStr("buildDir"))),
		intermediateExt(config.getStr("intermediateExt")),
		pchHeader(config.getStr("pch")),
		pchFile(buildDir + Path(config.getStr("pchFile"))),
		combinedPch(config.getBool("pchCompileCombined")) {

		linkOutput = config.getBool("linkOutput", false);
		forwardDeps = config.getBool("forwardDeps", false);

		vector<String> ign = config.getArray("ignore");
		for (nat i = 0; i < ign.size(); i++)
			ignore << Wildcard(ign[i]);

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
		DEBUG("Finding depencies for target in " << wd, INFO);
		toCompile.clear();

		CompileQueue q;
		String outputName = config.getStr("output");

		// Compile pre-compiled header first.
		String pchStr = config.getStr("pch");
		addFile(q, pchStr, true);

		// Add initial files.
		addFiles(q, config.getArray("input"));

		// Process files...
		while (q.any()) {
			Compile now = q.pop();

			if (!now.isPch && !now.autoFound && outputName.empty())
				outputName = now.title();

			// Check if 'now' is inside the working directory.
			if (!now.isChild(wd)) {
				// Maybe a reference to another target from the same project?
				Path parentRel = now.makeRelative(wd.parent());
				if (parentRel.first() != "..") {
					dependsOn << parentRel.first();
				}

				// No need to follow further!
				continue;
			}

			toCompile << now;

			// Add all other files we need.
			IncludeInfo info = includes.info(now);

			// Check so that any pch file is included first.
			if (!pchStr.empty() && pchStr != info.firstInclude) {
				PLN("ERROR: Precompiled header " << pchStr << " must be included first in each implementation file.");
				PLN("This is not the case for " << now << ".");
				PLN("You need to use '#include \"" << pchStr << "\"' (exactly like that), and use 'include=./'.");
				return false;
			}

			for (set<Path>::const_iterator i = info.includes.begin(); i != info.includes.end(); ++i) {
				DEBUG(now << " depends on " << *i, VERBOSE);
				addFile(q, *i);
			}
		}

		if (toCompile.empty()) {
			PLN("No input files.");
			return false;
		}

		if (outputName.empty())
			outputName = wd.title();

		Path execDir = wd + Path(config.getStr("execDir"));
		execDir.createDir();

		output = execDir + Path(outputName).titleNoExt();
		output.makeExt(config.getStr("execExt"));

		return true;
	}

	bool Target::compile() {
		DEBUG("Compiling target in " << wd, INFO);

		Path::cd(wd);
		DEBUG("CD into: " << wd, VERBOSE);

		Env env(config);

		// Run pre-compile steps.
		if (!runSteps("preBuild"))
			return false;

		map<String, String> data;
		data["file"] = "";
		data["output"] = "";
		data["pchFile"] = toS(pchFile.makeRelative(wd));

		Timestamp latestModified;
		ostringstream intermediateFiles;

		for (nat i = 0; i < toCompile.size(); i++) {
			const Compile &src = toCompile[i];
			Path output = src.makeRelative(wd).makeAbsolute(buildDir);
			output.makeExt(intermediateExt);

			output.parent().createDir();

			String file = toS(src.makeRelative(wd));
			String out = toS(output.makeRelative(wd));
			if (i > 0)
				intermediateFiles << ' ';
			intermediateFiles << out;

			Timestamp lastModified = includes.info(src).lastModified();

			bool pchValid = true;
			if (src.isPch) {
				pchValid = pchFile.exists() && pchFile.mTime() >= lastModified;
			}

			if (!force && pchValid && output.exists() && output.mTime() >= lastModified) {
				DEBUG("Skipping " << src.makeRelative(wd) << "...", INFO);
				DEBUG("Source modified: " << lastModified << ", output modified " << output.mTime(), VERBOSE);

				Timestamp mTime = output.mTime();
				if (i == 0)
					latestModified = mTime;
				else
					latestModified = max(latestModified, mTime);

				continue;
			}

			for (nat j = 0; j < ignore.size(); j++) {
				if (ignore[j].matches(file)) {
					DEBUG("Ignoring " << file << " as per " << ignore[j], INFO);
					continue;
				}
			}

			if (!combinedPch && src.isPch) {
				DEBUG("Compiling header " << src.makeRelative(wd) << "...", NORMAL);
				String cmd = config.getStr("pchCompile");
				data["file"] = pchHeader;
				data["output"] = data["pchFile"];
				cmd = config.expandVars(cmd, data);

				DEBUG("Command line: " << cmd, INFO);
				if (system(cmd.c_str()) != 0) {
					// Abort!
					return false;
				}
			}

			DEBUG("Compiling " << src.makeRelative(wd) << "...", NORMAL);

			String cmd;
			if (combinedPch && src.isPch)
				cmd = config.getStr("pchCompile");
			else
				cmd = chooseCompile(file);

			if (cmd == "") {
				PLN("No suitable compile command-line for " << file);
				return false;
			}

			data["file"] = file;
			data["output"] = out;
			cmd = config.expandVars(cmd, data);

			DEBUG("Command line: " << cmd, INFO);
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

		vector<String> libs = config.getArray("library");
		for (nat i = 0; i < libs.size(); i++) {
			Path libPath(libs[i]);
			// Ignore libraries we can not find (may be system libraries).
			if (libPath.exists())
				latestModified = max(latestModified, libPath.mTime());
		}

		// Link the output.
		if (!force && output.exists() && output.mTime() >= latestModified) {
			DEBUG("Skipping linking.", INFO);
			return true;
		}

		DEBUG("Linking " << output.title() << "...", NORMAL);

		data["files"] = intermediateFiles.str();
		data["output"] = toS(output.makeRelative(wd));
		String cmd = config.getVars("link", data);
		DEBUG("Command line: " << cmd, INFO);

		if (system(cmd.c_str()) != 0) {
			return false;
		}

		// Run pre-compile steps.
		if (!runSteps("preBuild"))
			return false;

		return true;
	}

	bool Target::runSteps(const String &key) {
		vector<String> steps = config.getArray(key);

		for (nat i = 0; i < steps.size(); i++) {
			String expanded = config.expandVars(steps[i]);
			if (system(expanded.c_str()) != 0) {
				PLN("Failed running " << key << ": " << expanded);
				return false;
			}
		}

		return true;
	}

	int Target::execute(const vector<String> &params) const {
		Path execPath = wd;
		if (config.has("execPath")) {
			execPath = Path(config.getStr("execPath")).makeAbsolute(wd);
		}

		Path::cd(execPath);
		DEBUG("Executing " << output.makeRelative(execPath) << " " << join(params) << " in " << execPath, INFO);
		return exec(output.makeRelative(execPath), params);
	}

	void Target::addLib(const Path &p) {
		config.add("localLibrary", toS(p.makeRelative(wd)));
	}

	void Target::addFiles(CompileQueue &to, const vector<String> &src) {
		for (nat i = 0; i < src.size(); i++) {
			addFile(to, src[i], false);
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

	void Target::addFilesRecursive(CompileQueue &to, const Path &at) {
		vector<Path> children = at.children();
		for (nat i = 0; i < children.size(); i++) {
			if (children[i].isDir()) {
				DEBUG("Found directory " << children[i], VERBOSE);
				addFilesRecursive(to, children[i]);
			} else {
				DEBUG("Found file " << children[i], VERBOSE);
				if (std::find(validExts.begin(), validExts.end(), children[i].ext()) != validExts.end()) {
					DEBUG("Adding file " << children[i], INFO);
					to << Compile(children[i], false, true);
				}
			}
		}
	}

	void Target::addFile(CompileQueue &to, const String &src, bool pch) {
		if (src.empty())
			return;

		if (src == "*") {
			addFilesRecursive(to, wd);
			return;
		}

		Path path(src);
		path = path.makeAbsolute(wd);

		if (!findExt(path)) {
			WARNING("The file " << src << " does not exist with any of the extensions " << join(validExts));
			return;
		}

		to.push(Compile(path, pch, false));
	}

	void Target::addFile(CompileQueue &to, const Path &header) {
		Path impl = header;
		if (!findExt(impl)) {
			// No cpp file for a header... No big deal.
			return;
		}

		to.push(Compile(impl, false, true));
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
