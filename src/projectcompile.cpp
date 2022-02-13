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
		while ((now = state.pop())) {
			if (mainTarget.empty())
				mainTarget = now->name;

			if (now->status == TargetInfo::sError) {
				DEBUG("Compilation of " << now << " failed!", NORMAL);
				return false;
			}

			Target *target = now->target;

			// Some targets are ignored. Handle that.
			if (!target)
				continue;

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
		while (info = work.pop()) {
			project->prepareTarget(info);
		}
	}

	vector<Path> Project::dependencies(const String &root) const {
		vector<Path> o;
		set<String> visited;
		visited.insert(root);
		dependencies(root, o, visited, root);
		return o;
	}

	void Project::dependencies(const String &root, vector<Path> &out, set<String> &visited, const String &at) const {
		map<String, TargetInfo *>::const_iterator atIt = target.find(at);
		if (atIt == target.end())
			return; // Should not happen.

		TargetInfo *t = atIt->second;

		for (set<String>::const_iterator i = t->depends.begin(); i != t->depends.end(); ++i) {
			const String &name = *i;

			if (visited.count(name))
				continue;
			visited.insert(name);

			map<String, TargetInfo *>::const_iterator f = target.find(name);
			if (f == target.end())
				continue;

			TargetInfo *z = f->second;
			if (!z || !z->target)
				continue;

			if (z->target->linkOutput) {
				DEBUG(root << " includes dependent " << z->target->output, INFO);
				out << z->target->output;
			}

			if (z->target->forwardDeps) {
				dependencies(root, out, visited, name);
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

		vector<Path> d = dependencies(info.name);
		for (nat i = 0; i < d.size(); i++) {
			t->target->addLib(d[i]);
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
			PLN("Nothing to run!");
			return 1;
		}

		return target[mainTarget]->target->execute(params);
	}


}
