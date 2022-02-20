#include "std.h"
#include "compile.h"
#include "wildcard.h"
#include "process.h"
#include "env.h"

namespace compile {

	// Find number of lines to skip in the output of a process. Checks if 'command' starts with <n>!
	// where <n> is an integer. Extracts the prefix and returns the rest of the string.
	nat extractSkip(String &command) {
		nat result = 0;
		for (nat i = 0; i < command.size(); i++) {
			char c = command[i];
			if (c >= '0' && c <= '9') {
				result = result*10 + (c - '0');
			} else if (c == '!') {
				// Done!
				command = command.substr(i + 1);
				return result;
			} else {
				// Something is not as expected, bail.
				break;
			}
		}

		return 0;
	}


	Target::Target(const Path &wd, const Config &config) :
		wd(wd),
		config(config),
		includes(wd, config),
		compileVariants(config.getArray("compile")),
		buildDir(wd + Path(config.getVars("buildDir"))),
		intermediateExt(config.getVars("intermediateExt")),
		pchHeader(config.getVars("pch")),
		pchFile(buildDir + Path(config.getVars("pchFile"))),
		combinedPch(config.getBool("pchCompileCombined")),
		appendExt(config.getBool("appendExt", false)),
		absolutePath(config.getBool("absolutePath", false)) {

		buildDir.makeDir();

		linkOutput = config.getBool("linkOutput", false);
		forwardDeps = config.getBool("forwardDeps", false);

		if (absolutePath) {
			vector<String> inc = this->config.getArray("include");
			this->config.clear("include");
			for (nat i = 0; i < inc.size(); i++) {
				this->config.add("include", toS(Path(inc[i]).makeAbsolute(wd)));
			}
		}

		// Add file extensions. We don't bother with uniqueness here, and let the ExtCache deal with that.
		validExts = config.getArray("ext");

		vector<String> ign = config.getArray("ignore");
		for (nat i = 0; i < ign.size(); i++)
			ignore << Wildcard(ign[i]);

		// Create build directory if it is not already created.
		buildDir.createDir();

		// Load cached data if possible.
		includes.ignore(config.getArray("noIncludes"));
		if (!force) {
			includes.load(buildDir + "includes");
			commands.load(buildDir + "commands");
		}
	}

	Target::~Target() {
		// We do this in "save", since if we execute the binary, the destructor will not be executed.
		// includes.save(buildDir + "includes");
	}

	void Target::clean() {
		DEBUG("Cleaning " << buildDir.makeRelative(wd) << "...", NORMAL);
		buildDir.recursiveDelete();

		Path e = wd + Path(config.getVars("execDir"));
		e.makeDir();
		DEBUG("Cleaning " << e.makeRelative(wd) << "...", NORMAL);
		e.recursiveDelete();
	}

	bool Target::find() {
		DEBUG("Finding dependencies for target in " << wd, VERBOSE);
		toCompile.clear();

		ExtCache cache(validExts);

		CompileQueue q;
		String outputName = config.getVars("output");

		// Compile pre-compiled header first.
		String pchStr = config.getVars("pch");
		if (!pchStr.empty()) {
			if (!addFile(q, cache, pchStr, true)) {
				PLN("Failed to find an implementation file for the precompiled header.");
				PLN("Make sure to create both a header file '" << pchStr << "' and a corresponding implementation file.");
				return false;
			}
		}

		// Add initial files.
		addFiles(q, cache, config.getArray("input"));

		// Process files...
		while (q.any()) {
			Compile now = q.pop();

			if (ignored(toS(now.makeRelative(wd))))
				continue;

			if (!now.isPch && !now.autoFound && outputName.empty())
				outputName = now.title();

			// Check if 'now' is inside the working directory.
			if (!now.isChild(wd)) {
				// No need to follow further!
				DEBUG("Ignoring file outside of working directory: " << now.makeRelative(wd), VERBOSE);
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

			for (IncludeInfo::PathSet::const_iterator i = info.includes.begin(); i != info.includes.end(); ++i) {
				DEBUG(now << " depends on " << *i, VERBOSE);
				addFile(q, cache, *i);

				// Is this a reference to another sub-project?
				if (!i->isChild(wd)) {
					Path parentRel = i->makeRelative(wd.parent());
					if (parentRel.first() != "..") {
						dependsOn << parentRel.first();
					}
				}
			}
		}

		if (toCompile.empty()) {
			PLN("No input files.");
			return false;
		}

		// Try to add the files which should be created by the pre-build step (if any).
		addPreBuildFiles(q, config.getArray("preBuildCreates"));
		while (q.any()) {
			Compile now = q.pop();
			DEBUG("Adding file created by pre-build step currently not existing: " << now.makeRelative(wd), INFO);
			toCompile << now;
		}

		if (outputName.empty())
			outputName = wd.title();

		Path execDir = wd + Path(config.getVars("execDir"));
		execDir.createDir();

		output = execDir + Path(outputName).titleNoExt();
		output.makeExt(config.getStr("execExt"));

		return true;
	}

	// Save command line on exit.
	class SaveOnExit : public ProcessCallback {
	public:
		SaveOnExit(Commands *to, const String &file, const String &command) : to(to), key(file), command(command) {}

		// Save to.
		Commands *to;

		// Source file used as key.
		String key;

		// Command line.
		String command;

		virtual void exited(int result) {
			if (result == 0)
				to->set(key, command);
		}
	};

	Process *Target::saveShellProcess(const String &file, const String &command, const Path &cwd, const Env *env, nat skip) {
		Process *p = shellProcess(command, cwd, env, skip);
		p->callback = new SaveOnExit(&commands, file, command);
		return p;
	}

	bool Target::compile() {
		nat threads = to<nat>(config.getStr("maxThreads", "1"));

		// Force serial execution?
		if (!config.getBool("parallel", true))
			threads = 1;

		DEBUG("Using max " << threads << " threads.", VERBOSE);
		ProcGroup group(threads, outputState);

		DEBUG("Compiling target in " << wd, INFO);

		Env env(config);

		// Run pre-compile steps.
		{
			map<String, String> stepData;
			stepData["output"] = toS(output.makeRelative(wd));
			if (!runSteps("preBuild", group, env, stepData))
				return false;
		}

		map<String, String> data;
		data["file"] = "";
		data["output"] = "";
		data["pchFile"] = toS(pchFile.makeRelative(wd));

		TimeCache timeCache;
		Timestamp latestModified(0);
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

			Timestamp lastModified = includes.info(src).lastModified(timeCache);

			bool pchValid = true;
			if (src.isPch) {
				// Note: This implies that pchFile exists as well.
				pchValid = pchFile.mTime() >= lastModified;
			}

			bool skip = !force     // Never skip a file if the force flag is set.
				&& pchValid        // If the pch is invalid, don't skip.
				&& output.mTime() >= lastModified; // If the output exists and is new enough, we can skip.

			if (!combinedPch && src.isPch) {
				String cmd = config.getStr("pchCompile");
				Path pchPath = Path(pchHeader).makeAbsolute(wd);
				String pchFile = toS(pchPath.makeRelative(wd));
				data["file"] = preparePath(pchPath);
				data["output"] = data["pchFile"];
				cmd = config.expandVars(cmd, data);

				nat skipLines = extractSkip(cmd);

				if (skip && commands.check(pchFile, cmd)) {
					DEBUG("Skipping header " << file << "...", VERBOSE);
				} else {
					DEBUG("Compiling header " << file << "...", NORMAL);
					DEBUG(cmd, COMMAND);
					if (!group.spawn(saveShellProcess(pchFile, cmd, wd, &env, skipLines))) {
						return false;
					}

					// Wait for it to complete...
					if (!group.wait()) {
						return false;
					}
				}
			}

			String cmd;
			if (combinedPch && src.isPch)
				cmd = config.getStr("pchCompile");
			else
				cmd = chooseCompile(file);

			if (cmd == "") {
				PLN("No suitable compile command-line for " << file);
				return false;
			}

			data["file"] = preparePath(src);
			data["output"] = out;
			cmd = config.expandVars(cmd, data);

			nat skipLines = extractSkip(cmd);

			if (skip && commands.check(file, cmd)) {
				DEBUG("Skipping " << file << "...", VERBOSE);
				DEBUG("Source modified: " << lastModified << ", output modified " << output.mTime(), DEBUG);
			} else {
				DEBUG("Compiling " << file << "...", NORMAL);
				DEBUG(cmd, COMMAND);
				if (!group.spawn(saveShellProcess(file, cmd, wd, &env, skipLines)))
					return false;

				// If it is a pch, wait for it to finish.
				if (src.isPch) {
					if (!group.wait())
						return false;
				}
			}

			// Update 'last modified'. We always want to do this, even if we did not need to compile the file.
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
			if (!libPath.isAbsolute())
				libPath = libPath.makeAbsolute(wd);
			if (libPath.exists()) {
				latestModified = max(latestModified, libPath.mTime());
			} else {
				WARNING("Local library " << libPath << " not found. Use 'library' for system libraries.");
			}
		}

		// Link the output.
		bool skipLink = !force && output.mTime() >= latestModified;

		String finalOutput = toS(output.makeRelative(wd));
		data["files"] = intermediateFiles.str();
		data["output"] = finalOutput;

		vector<String> linkCmds = config.getArray("link");
		vector<nat> linkSkip;
		std::ostringstream allCmds;
		for (nat i = 0; i < linkCmds.size(); i++) {
			linkCmds[i] = config.expandVars(linkCmds[i], data);
			linkSkip.push_back(extractSkip(linkCmds[i]));
			if (i > 0)
				allCmds << ";";
			allCmds << linkCmds[i];
		}

		if (skipLink && commands.check(finalOutput, allCmds.str())) {
			DEBUG("Skipping linking.", VERBOSE);
			DEBUG("Output modified " << output.mTime() << ", input modified " << latestModified, DEBUG);
			return true;
		}

		DEBUG("Linking " << output.title() << "...", NORMAL);

		for (nat i = 0; i < linkCmds.size(); i++) {
			const String &cmd = linkCmds[i];
			DEBUG(cmd, COMMAND);

			if (!group.spawn(shellProcess(cmd, wd, &env, linkSkip[i])))
				return false;

			if (!group.wait())
				return false;
		}

		commands.set(finalOutput, allCmds.str());

		{
			// Run post-build steps.
			map<String, String> stepData;
			stepData["output"] = data["output"];
			if (!runSteps("postBuild", group, env, stepData))
				return false;
		}

		return true;
	}

	String Target::preparePath(const Path &file) {
		if (absolutePath) {
			return toS(file.makeAbsolute(wd));
		} else {
			return toS(file.makeRelative(wd));
		}
	}

	bool Target::ignored(const String &file) {
		for (nat j = 0; j < ignore.size(); j++) {
			if (ignore[j].matches(file)) {
				DEBUG("Ignoring " << file << " as per " << ignore[j], VERBOSE);
				return true;
			}
		}
		return false;
	}

	bool Target::runSteps(const String &key, ProcGroup &group, const Env &env, const map<String, String> &options) {
		vector<String> steps = config.getArray(key);

		for (nat i = 0; i < steps.size(); i++) {
			String expanded = config.expandVars(steps[i], options);
			nat skip = extractSkip(expanded);
			DEBUG(expanded, COMMAND);
			if (!group.spawn(shellProcess(expanded, wd, &env, skip))) {
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

	void Target::save() const {
		includes.save(buildDir + "includes");
		commands.save(buildDir + "commands");
	}

	int Target::execute(const vector<String> &params) const {
		if (!config.getBool("execute"))
			return 0;

		Path execPath = wd;
		if (config.has("execPath")) {
			execPath = Path(config.getStr("execPath")).makeAbsolute(wd);
		}

		DEBUG(output.makeRelative(execPath) << " " << join(params) << " in " << execPath, COMMAND);
		return exec(output, params, execPath, null);
	}

	void Target::addLib(const Path &p) {
		if (!absolutePath && p.isAbsolute()) {
			config.add("localLibrary", toS(p.makeRelative(wd)));
		} else {
			config.add("localLibrary", toS(p));
		}
	}

	vector<Path> Target::findExt(const Path &path, ExtCache &cache) {
		// Earlier implementation did way too many queries for files, and has therefore been optimized.
		// for (nat i = 0; i < validExts.size(); i++) {
		// 	path.makeExt(validExts[i]);
		// 	if (path.exists())
		// 		return true;
		// }

		const vector<String> &exts = cache.find(path);

		vector<Path> result;
		for (nat i = 0; i < exts.size(); i++) {
			result.push_back(path);
			result.back().makeExt(exts[i]);
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

	void Target::addFiles(CompileQueue &to, ExtCache &cache, const vector<String> &src) {
		for (nat i = 0; i < src.size(); i++) {
			addFile(to, cache, src[i], false);
		}
	}

	void Target::addFilesRecursive(CompileQueue &to, const Path &at) {
		vector<Path> children = at.children();
		for (nat i = 0; i < children.size(); i++) {
			if (children[i].isDir()) {
				DEBUG("Found directory " << children[i], DEBUG);
				addFilesRecursive(to, children[i]);
			} else {
				DEBUG("Found file " << children[i], DEBUG);
				if (std::find(validExts.begin(), validExts.end(), children[i].ext()) != validExts.end()) {
					DEBUG("Adding file " << children[i], VERBOSE);
					to << Compile(children[i], false, true);
				}
			}
		}
	}

	bool Target::addFile(CompileQueue &to, ExtCache &cache, const String &src, bool pch) {
		if (src.empty())
			return false;

		if (src == "*") {
			addFilesRecursive(to, wd);
			return true;
		}

		Path path(src);
		path = path.makeAbsolute(wd);

		vector<Path> exts = findExt(path, cache);
		if (exts.empty()) {
			WARNING("The file " << src << " does not exist with any of the extensions " << join(validExts));
			return false;
		}

		for (nat i = 0; i < exts.size(); i++) {
			to.push(Compile(exts[i], pch, false));
		}

		return true;
	}

	void Target::addFile(CompileQueue &to, ExtCache &cache, const Path &header) {
		vector<Path> exts = findExt(header, cache);

		// No big deal if no cpp file is found for every h file.
		for (nat i = 0; i < exts.size(); i++) {
			to.push(Compile(exts[i], false, true));
		}
	}

	void Target::addPreBuildFiles(CompileQueue &to, const vector<String> &src) {
		for (nat i = 0; i < src.size(); i++)
			addPreBuildFile(to, src[i]);
	}

	void Target::addPreBuildFile(CompileQueue &to, const String &src) {
		Path path = Path(src).makeAbsolute(wd);
		to.push(Compile(path, false, true));
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

		DEBUG("Failed to find a compilation fommand for " << file, NORMAL);
		return "";
	}

}
