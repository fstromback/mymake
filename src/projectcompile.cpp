#include "std.h"
#include "projectcompile.h"
#include "thread.h"
#include "atomic.h"

namespace compile {

	Project::Project(const Path &wd, const set<String> &cmdline, const MakeConfig &projectFile, const Config &config, bool showTimes) :
		wd(wd),
		projectFile(projectFile),
		config(config),
		showTimes(showTimes) {

		{
			set<String> s = cmdline;
			s.insert("deps");
			projectFile.apply(s, depsConfig);
		}
		{
			set<String> s = cmdline;
			s.insert("build");
			projectFile.apply(s, buildConfig);
		}
		explicitTargets = config.getBool("explicitTargets", false);
		implicitDependencies = config.getBool("implicitDeps", true);

#ifdef WINDOWS
		usePrefix = "vc";
#else
		usePrefix = "gnu";
#endif
		usePrefix = config.getStr("usePrefix", usePrefix);

		// Number of threads to use.
		numThreads = to<nat>(config.getStr("maxThreads", "1"));

		// Force serial execution?
		if (!config.getBool("parallel", true))
			numThreads = 1;

	}

	Project::~Project() {
		for (map<String, TargetInfo *>::iterator i = target.begin(); i != target.end(); ++i) {
			delete i->second;
		}
	}

	bool Project::find() {
		// Figure out which projects we need.
		FindState state(this);
		addTargets(config.getArray("input"), state);

		TargetInfo *now;
		while ((now = state.pop()) != null) {

			if (now->status == TargetInfo::sError) {
				DEBUG("Compilation of " << now->name << " failed!", NORMAL);
				return false;
			}

			Target *target = now->target;

			// Some targets are ignored. If so, see if our config contains the option at all. If
			// not, we need to warn about it.
			if (!target) {
				if (projectFile.options().count(now->name) == 0) {
					WARNING("Ignoring the option " << now->name << " since it is neither the "
							<< "name of a target, nor the name of a configuration option in the "
							<< "project file.");
				}
				continue;
			}

			// Remember which target is the main one. Ignore non-targets.
			if (mainTarget.empty())
				mainTarget = now->name;

			// Add any dependent projects.
			if (implicitDependencies) {
				for (set<String>::const_iterator i = target->dependsOn.begin(); i != target->dependsOn.end(); ++i) {
					addTarget(*i, state);
					now->depends << *i;
				}
			}

			// Add any explicit dependent projects.
			vector<String> depends = depsConfig.getArray(now->name);
			for (nat i = 0; i < depends.size(); i++) {
				addTarget(depends[i], state);
				now->depends << depends[i];
			}

			order << now->node();

			DEBUG(now->name << " depends on " << join(now->depends, ", "), INFO);
		}

		// Ask threads to exit.
		state.done();

		if (order.empty()) {
			DEBUG("No input files or projects. Compilation failed.", NORMAL);
			return false;
		}

		try {
			order = topoSort(order);
		} catch (const TopoError &e) {
			PLN("Error: " << e.what());
			return false;
		}

		// Update the 'order' part of all targets.
		for (nat i = 0; i < order.size(); i++) {
			target[order[i].name]->order = i;
		}

		return true;
	}

	void Project::prepareTarget(TargetInfo *info) const {
		if (atomicRead(info->status) != TargetInfo::sNotReady)
			return;

		DEBUG("Examining target " << info->name, INFO);

		info->target = loadTarget(info->name);
		if (info->target) {
			if (info->target->find())
				atomicWrite(info->status, TargetInfo::sOK);
			else
				atomicWrite(info->status, TargetInfo::sError);

		} else {
			// We don't always treat all subdirectories as targets.
			atomicWrite(info->status, TargetInfo::sOK);
		}

		info->done.signal();
	}

	void Project::addTargets(const vector<String> &params, FindState &to) {
		for (nat i = 0; i < params.size(); i++)
			addTarget(params[i], to);
	}

	void Project::addTarget(const String &param, FindState &to) {
		Path rel = Path(param);
		if (rel.isAbsolute())
			rel = rel.makeRelative(wd);
		if (rel.isEmpty())
			return;

		String name = rel.first();
		// Duplicate?
		if (target.count(name))
			return;

		TargetInfo *info = new TargetInfo(name);
		target.insert(make_pair(name, info));
		to.push(info);
	}

	Target *Project::loadTarget(const String &name) const {
		Path dir = wd + name;
		dir.makeDir();

		if (!dir.exists()) {
			DEBUG(name << " is not a sub-project. The directory " << dir << " does not exist.", PEDANTIC);
			return null;
		}

		vector<String> vOptions = buildConfig.getArray(name);
		set<String> options(vOptions.begin(), vOptions.end());

		vOptions = buildConfig.getArray("all");
		options.insert(vOptions.begin(), vOptions.end());

		DEBUG("Options for " << name << ": " << join(options), VERBOSE);

		MakeConfig config;

		Path configFile = dir + localConfig;
		if (configFile.exists()) {
			DEBUG("Found local config: " << configFile, VERBOSE);
			config.load(configFile);
		} else if (explicitTargets) {
			DEBUG("No config in " << dir << ", ignoring it since 'explicitTargets=1'.", PEDANTIC);
			return null;
		} else {
			DEBUG("No config in " << dir << ", using it anyway since 'explicitTargets=0'.", PEDANTIC);
		}

		Config opt;
		// Add output path from our config.
		String execPath = this->config.getStr("execPath");
		if (!execPath.empty()) {
			opt.set("execPath", execPath);
		}

		// Add input files from our config.
		vector<String> input = this->config.getArray("input");
		for (nat i = 0; i < input.size(); i++) {
			Path abs = Path(input[i]).makeAbsolute(wd);
			if (abs.isChild(dir)) {
				opt.add("input", toS(abs));
			}
		}

		// Add project path and local path.
		opt.add("projectRoot", toS(wd));
		opt.add("targetRoot", toS(dir));

		projectFile.apply(options, opt);
		config.apply(options, opt);

		DEBUG("Configuration for " << name << ": " << opt, VERBOSE);

		return new Target(dir, opt);
	}

	Project::FindState::FindState(Project *p) : project(p), threads(null) {
		if (project->numThreads > 1) {
			threads = new Thread[p->numThreads];
			for (nat i = 0; i < project->numThreads; i++) {
				threads[i].start(&FindState::main, *this);
			}
		}
	}

	Project::FindState::~FindState() {
		// Failsafe.
		work.done();

		if (threads) {
			for (nat i = 0; i < project->numThreads; i++)
				threads[i].join();
			delete []threads;
		}
	}

	void Project::FindState::push(TargetInfo *info) {
		process.push(info);
		work.push(info);
	}

	Project::TargetInfo *Project::FindState::pop() {
		if (process.empty())
			return null;

		TargetInfo *r = process.front();
		process.pop();

		// Ensure it is processed.
		if (isMT()) {
			r->done.wait();
		} else {
			project->prepareTarget(r);
		}

		return r;
	}

	void Project::FindState::done() {
		work.done();
	}

	void Project::FindState::main() {
		TargetInfo *info = null;
		while ((info = work.pop()) != null) {
			project->prepareTarget(info);
		}
	}

	map<nat, Path> Project::dependencies(const String &root) const {
		map<nat, Path> output;
		vector<bool> visited(order.size(), false);

		map<String, TargetInfo *>::const_iterator found = target.find(root);
		if (found == target.end())
			return output; // Should not happen...

		visited[found->second->order] = true;

		dependencies(root, visited, output, found->second);

		return output;
	}

	void Project::dependencies(const String &root, vector<bool> &visited, map<nat, Path> &output, const TargetInfo *at) const {
		for (set<String>::const_iterator i = at->depends.begin(); i != at->depends.end(); ++i) {
			const String &name = *i;

			map<String, TargetInfo *>::const_iterator f = target.find(name);
			if (f == target.end())
				continue; // Should not happen...

			TargetInfo *info = f->second;

			// Exclude duplicates. Won't hurt, but is expensive.
			if (visited[info->order])
				continue;
			visited[info->order] = true;

			if (!info->target)
				continue;

			if (info->target->linkOutput) {
				DEBUG(root << " includes dependent " << info->target->output, INFO);
				output[info->order] = info->target->output;
			}

			if (info->target->forwardDeps) {
				dependencies(root, visited, output, info);
			}
		}
	}

	void Project::clean() {
		for (nat i = 0; i < order.size(); i++) {
			TargetDeps &info = order[i];
			Target *t = target[info.name]->target;
			DEBUG("-- Target " << info.name << " --", NORMAL);
			t->clean();
		}
	}

	bool Project::compile() {
		if (numThreads <= 1) {
			return compileST();
		} else {
			return compileMT(numThreads);
		}
	}

	bool Project::compileOne(nat id, bool mt) {
		String prefix;
		if (mt) {
			if (usePrefix == "vc") {
				prefix = toS(id + 1) + ">";
			} else if (usePrefix == "gnu") {
				prefix = "p" + toS(id + 1) + ": ";
			}
		}

		TargetDeps &info = order[id];
		TargetInfo *t = target[info.name];

		String banner;
		if (debugLevel >= dbg_NORMAL)
			banner = "-- Target " + info.name + " --";
		SetBanner w(banner.c_str());
		SetPrefix z(prefix.c_str());

		Timestamp start;

		map<nat, Path> d = dependencies(info.name);
		for (map<nat, Path>::reverse_iterator i = d.rbegin(), end = d.rend(); i != end; ++i) {
			t->target->addLib(i->second);
		}

		bool ok = t->target->compile();
		Timestamp end;
		if (showTimes)
			PLN("Compilation time (" << info.name << "): " << (end - start));

		if (!ok) {
			DEBUG("Compilation of " << info.name << " failed!", NORMAL);
			return false;
		}

		return true;
	}

	bool Project::compileST() {
		for (nat i = 0; i < order.size(); i++) {
			if (!compileOne(i, false))
				return false;
		}

		return true;
	}

	bool Project::compileMT(nat threadCount) {
		MTCompileState state(*this);

		Thread *threads = new Thread[threadCount];
		for (nat i = 0; i < threadCount; i++) {
			threads[i].start(&MTCompileState::start, state);
		}

		// Join all threads, and read result.
		for (nat i = 0; i < threadCount; i++) {
			threads[i].join();
		}

		return atomicRead(state.ok) != 0;
	}

	Project::MTCompileState::MTCompileState(Project &p) :
		p(&p), next(0), ok(1) {

		for (nat i = 0; i < p.order.size(); i++)
			targetDone.insert(make_pair(p.order[i].name, new Condition()));
	}

	Project::MTCompileState::~MTCompileState() {
		for (map<String, Condition *>::iterator i = targetDone.begin(), end = targetDone.end(); i != end; ++i) {
			delete i->second;
		}
	}

	void Project::MTCompileState::start() {
		if (!p->threadMain(*this)) {
			if (!atomicRead(ok))
				return;

			// This does not matter too much if we do it more than once.
			atomicWrite(ok, 0);

			// Signal all condition variables to cause threads to exit.
			for (map<String, Condition *>::iterator i = targetDone.begin(), end = targetDone.end(); i != end; ++i) {
				i->second->signal();
			}
		}
	}

	bool Project::threadMain(MTCompileState &state) {
		while (true) {
			nat work = atomicInc(state.next);

			// Done?
			if (work >= order.size())
				break;

			// Wait until all dependencies are satisfied.
			TargetDeps &info = order[work];
			for (set<String>::const_iterator i = info.dependsOn.begin(), end = info.dependsOn.end(); i != end; ++i) {
				map<String, Condition *>::iterator f = state.targetDone.find(*i);

				// Note: some dependencies are ignored. That is fine!
				if (f == state.targetDone.end())
					continue;

				f->second->wait();
			}

			// Double-check so that something did not fail.
			if (!atomicRead(state.ok))
				return false;

			if (!compileOne(work, true))
				return false;

			map<String, Condition *>::iterator f = state.targetDone.find(info.name);
			if (f == state.targetDone.end())
				return false;
			f->second->signal();

			// Once again, something failed?
			if (!atomicRead(state.ok))
				return false;
		}

		return true;
	}

	void Project::save() const {
		for (map<String, TargetInfo *>::const_iterator i = target.begin(); i != target.end(); ++i) {
			if (i->second && i->second->target)
				i->second->target->save();
		}
	}

	int Project::execute(const vector<String> &params) {
		if (mainTarget.empty()) {
			PLN("No default project specified, nothing to run!");
			return 1;
		}

		map<String, TargetInfo *>::const_iterator i = target.find(mainTarget);
		if (i == target.end()) {
			PLN("The first parameter, " << mainTarget << ", does not denote a target. Nothing to run!");
			return 1;
		}

		if (!i->second->target) {
			PLN("The first parameter, " << mainTarget << ", does not denote a target. Nothing to run!");
			return 1;
		}

		return i->second->target->execute(params);
	}


}
