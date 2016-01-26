#include "std.h"
#include "compile.h"
#include "wildcard.h"
#include "process.h"
#include "env.h"

namespace compile {

	Target::Target(const Path &wd, const Config &config) :
		wd(wd),
		config(config),
		includes(wd, config),
		compileVariants(config.getArray("compile")),
		buildDir(wd + Path(config.getStr("buildDir"))),
		intermediateExt(config.getStr("intermediateExt")),
		pchHeader(config.getStr("pch")),
		pchFile(buildDir + Path(config.getStr("pchFile"))),
		combinedPch(config.getBool("pchCompileCombined")),
		appendExt(config.getBool("appendExt", false)) {

		buildDir.makeDir();

		linkOutput = config.getBool("linkOutput", false);
		forwardDeps = config.getBool("forwardDeps", false);

		// Add unique file extensions.
		vector<String> ext = config.getArray("ext");
		set<String> extSet;
		for (nat i = 0; i < ext.size(); i++) {
			if (extSet.count(ext[i]) == 0) {
				extSet.insert(ext[i]);
				validExts << ext[i];
			}
		}

		vector<String> ign = config.getArray("ignore");
		for (nat i = 0; i < ign.size(); i++)
			ignore << Wildcard(ign[i]);

		// Create build directory if it is not already created.
		buildDir.createDir();

		// Load cached data if possible.
		includes.ignore(config.getArray("noIncludes"));
		if (!force) {
			includes.load(buildDir + "includes");
		}
	}

	Target::~Target() {
		includes.save(buildDir + "includes");
	}

	void Target::clean() {
		DEBUG("Cleaning " << buildDir.makeRelative(wd) << "...", NORMAL);
		buildDir.recursiveDelete();

		Path e = wd + Path(config.getStr("execDir"));
		e.makeDir();
		DEBUG("Cleaning " << e.makeRelative(wd) << "...", NORMAL);
		e.recursiveDelete();
	}

	bool Target::find() {
		DEBUG("Finding dependencies for target in " << wd, INFO);
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

			if (ignored(toS(now.makeRelative(wd))))
				continue;

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
			if (!info.ignored && !pchStr.empty() && pchStr != info.firstInclude) {
				PLN(now << ":1: Precompiled header " << pchStr << " not used.");
				PLN("The precompiled header " << pchStr << " must be included first in each implementation file.");
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

	bool Target::compile(const String &prefix) {
		nat threads = to<nat>(config.getStr("maxThreads", "1"));

		// Force serial execution?
		if (!config.getBool("parallel", true))
			threads = 1;

		DEBUG("Using max " << threads << " threads.", VERBOSE);
		ProcGroup group(threads, prefix);

		DEBUG("Compiling target in " << wd, INFO);

		Env env(config);

		// Run pre-compile steps.
		if (!runSteps("preBuild", group, env, map<String, String>()))
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

			if (appendExt) {
				String t = output.titleNoExt() + "_" + output.ext() + "." + intermediateExt;
				output.makeTitle(t);
			} else {
				output.makeExt(intermediateExt);
			}

			output.parent().createDir();

			String file = toS(src.makeRelative(wd));
			String out = toS(output.makeRelative(wd));
			if (i > 0)
				intermediateFiles << ' ';
			intermediateFiles << out;

			if (ignored(file))
				continue;

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

			if (!combinedPch && src.isPch) {
				DEBUG("Compiling header " << src.makeRelative(wd) << "...", NORMAL);
				String cmd = config.getStr("pchCompile");
				data["file"] = pchHeader;
				data["output"] = data["pchFile"];
				cmd = config.expandVars(cmd, data);

				DEBUG("Command line: " << cmd, INFO);
				if (!group.spawn(shellProcess(cmd, wd, &env))) {
					return false;
				}

				// Wait for it to complete...
				if (!group.wait()) {
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
			if (!group.spawn(shellProcess(cmd, wd, &env)))
				return false;

			// If it is a pch, wait for it to finish.
			if (src.isPch) {
				if (!group.wait())
					return false;
			}

			// Update 'last modified'
			if (i == 0)
				latestModified = lastModified;
			else
				latestModified = max(latestModified, lastModified);

		}

		// Wait for compilation to terminate.
		if (!group.wait())
			return false;

		vector<String> libs = config.getArray("localLibrary");
		for (nat i = 0; i < libs.size(); i++) {
			Path libPath(libs[i]);
			if (libPath.exists()) {
				latestModified = max(latestModified, libPath.mTime());
			} else {
				WARNING("Local library " << libPath << " not found. Use 'library' for system libraries.");
			}
		}

		// Link the output.
		if (!force && output.exists() && output.mTime() >= latestModified) {
			DEBUG("Skipping linking.", INFO);
			DEBUG("Output modified " << output.mTime() << ", input modified " << latestModified, VERBOSE);
			return true;
		}

		DEBUG("Linking " << output.title() << "...", NORMAL);

		data["files"] = intermediateFiles.str();
		data["output"] = toS(output.makeRelative(wd));

		vector<String> linkCmds = config.getArray("link");
		for (nat i = 0; i < linkCmds.size(); i++) {
			String cmd = config.expandVars(linkCmds[i], data);
			DEBUG("Command line: " << cmd, INFO);

			if (!group.spawn(shellProcess(cmd, wd, &env)))
				return false;

			if (!group.wait())
				return false;
		}

		// Run post-compile steps.
		map<String, String> stepData;
		stepData["output"] = data["output"];
		if (!runSteps("postBuild", group, env, stepData))
			return false;

		return true;
	}

	bool Target::ignored(const String &file) {
		for (nat j = 0; j < ignore.size(); j++) {
			if (ignore[j].matches(file)) {
				DEBUG("Ignoring " << file << " as per " << ignore[j], INFO);
				return true;
			}
		}
		return false;
	}

	bool Target::runSteps(const String &key, ProcGroup &group, const Env &env, const map<String, String> &options) {
		vector<String> steps = config.getArray(key);

		for (nat i = 0; i < steps.size(); i++) {
			String expanded = config.expandVars(steps[i], options);
			DEBUG("Running " << expanded, INFO);
			if (!group.spawn(shellProcess(expanded, wd, &env))) {
				PLN("Failed running " << key << ": " << expanded);
				return false;
			}

			if (!group.wait()) {
				PLN("Failed running " << key << ": " << expanded);
				return false;
			}
		}

		return true;
	}

	int Target::execute(const vector<String> &params) const {
		if (!config.getBool("execute"))
			return 0;

		Path execPath = wd;
		if (config.has("execPath")) {
			execPath = Path(config.getStr("execPath")).makeAbsolute(wd);
		}

		DEBUG("Executing " << output.makeRelative(execPath) << " " << join(params) << " in " << execPath, INFO);
		return exec(output, params, execPath, null);
	}

	void Target::addLib(const Path &p) {
		config.add("localLibrary", toS(p));
	}

	void Target::addFiles(CompileQueue &to, const vector<String> &src) {
		for (nat i = 0; i < src.size(); i++) {
			addFile(to, src[i], false);
		}
	}

	Target::CacheItem Target::buildCache(const Path &path) const {
		CacheItem r;

		vector<Path> children = path.children();
		for (nat i = 0; i < children.size(); i++) {
			Path &p = children[i];

			if (p.isDir())
				continue;

			Path title(p.titleNoExt());
			r[title] << p.ext();
		}

		return r;
	}

	vector<Path> Target::findExt(const Path &path) {
		// Earlier implementation did way too many queries for files, and has therefore been optimized.
		// for (nat i = 0; i < validExts.size(); i++) {
		// 	path.makeExt(validExts[i]);
		// 	if (path.exists())
		// 		return true;
		// }

		// This is faster since we do not need to ask the OS as many times in large projects.
		Path title(path.titleNoExt());
		Path parent = path.parent();
		vector<Path> result;

		CacheMap::const_iterator i = findExtCache.find(parent);
		if (i == findExtCache.end()) {
			i = findExtCache.insert(make_pair(parent, buildCache(parent))).first;
		}

		CacheItem::const_iterator j = i->second.find(title);

		// No such file.
		if (j == i->second.end())
			return result;

		const vector<String> &exts = j->second;
		for (nat f = 0; f < exts.size(); f++) {
			for (nat i = 0; i < validExts.size(); i++) {
				if (Path::equal(exts[f], validExts[i])) {
					result.push_back(path);
					result.back().makeExt(validExts[i]);
				}
			}
		}

		if (!appendExt && result.size() > 1) {
			WARNING("Multiple files with the same name scheduled for compilation.");
			PLN("This leads to a name collision in the temporary files, leading to");
			PLN("incorrect compilation, and probably also linker errors.");
			PLN("If you intend to compile all files below, add 'appendExt=yes' to this target.");
			PLN("Colliding files:\n" << join(result, "\n"));
		}

		return result;
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

		vector<Path> exts = findExt(path);
		if (exts.empty()) {
			WARNING("The file " << src << " does not exist with any of the extensions " << join(validExts));
			return;
		}

		for (nat i = 0; i < exts.size(); i++) {
			to.push(Compile(exts[i], pch, false));
		}
	}

	void Target::addFile(CompileQueue &to, const Path &header) {
		vector<Path> exts = findExt(header);

		// No big deal if no cpp file is found for every h file.
		for (nat i = 0; i < exts.size(); i++) {
			to.push(Compile(exts[i], false, true));
		}
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
