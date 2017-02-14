#include "std.h"
#include "projectcompile.h"
#include "thread.h"
#include "atomic.h"

namespace compile {

	set<String> createSet(const String &member) {
		set<String> s;
		s << member;
		return s;
	}

	Project::Project(const Path &wd, const set<String> &cmdline, const MakeConfig &projectFile, const Config &config) :
		wd(wd),
		projectFile(projectFile),
		config(config) {

		projectFile.apply(createSet("deps") + cmdline, depsConfig);
		projectFile.apply(createSet("build") + cmdline, buildConfig);
		explicitTargets = config.getBool("explicitTargets", false);
		implicitDependencies = config.getBool("implicitDeps", true);

#ifdef WINDOWS
		usePrefix = "vc";
#else
		usePrefix = "gnu";
#endif
		usePrefix = config.getStr("usePrefix", usePrefix);
	}

	Project::~Project() {
		for (map<String, Target *>::iterator i = target.begin(); i != target.end(); ++i) {
			delete i->second;
		}
	}

	bool Project::find() {
		// Figure out which projects we need.
		TargetQueue q;
		addTargets(config.getArray("input"), q);

		while (q.any()) {
			String now = q.pop();
			DEBUG("Examining target " << now, INFO);

			if (mainTarget.empty())
				mainTarget = now;

			Target *target = loadTarget(now);
			if (!target)
				continue;

			this->target[now] = target;
			if (!target->find()) {
				DEBUG("Compilation of " << now << " failed!", NORMAL);
				return false;
			}

			TargetInfo info = { now, set<String>() };

			// Add any dependent projects.
			if (implicitDependencies) {
				for (set<String>::const_iterator i = target->dependsOn.begin(); i != target->dependsOn.end(); ++i) {
					q << *i;
					info.dependsOn << *i;
				}
			}

			// Add any explicit dependent projects.
			vector<String> depends = depsConfig.getArray(now);
			for (nat i = 0; i < depends.size(); i++) {
				q << depends[i];
				info.dependsOn << depends[i];
			}

			order << info;
			targetInfo[info.name] = info;

			DEBUG("Done. " << now << " depends on " << join(info.dependsOn, ", "), INFO);
		}

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

	vector<Path> Project::dependencies(const String &root, const TargetInfo &at) const {
		vector<Path> o;
		dependencies(root, o, at);
		return o;
	}

	void Project::dependencies(const String &root, vector<Path> &out, const TargetInfo &at) const {
		for (set<String>::const_iterator i = at.dependsOn.begin(); i != at.dependsOn.end(); ++i) {
			const String &name = *i;

			map<String, Target *>::const_iterator f = target.find(name);
			if (f == target.end())
				continue;

			Target *z = f->second;
			if (z->linkOutput) {
				DEBUG(root << " includes dependent " << z->output, INFO);
				out << z->output;
			}

			if (z->forwardDeps) {
				map<String, TargetInfo>::const_iterator f = targetInfo.find(name);
				if (f == targetInfo.end())
					continue;

				dependencies(root, out, f->second);
			}
		}
	}

	static vector<Path> removeDuplicates(const vector<Path> &d) {
		map<Path, nat> count;
		for (nat i = 0; i < d.size(); i++) {
			count[d[i]]++;
		}

		vector<Path> r;
		for (nat i = 0; i < d.size(); i++) {
			if (--count[d[i]] == 0)
				r << d[i];
		}

		return r;
	}

	void Project::clean() {
		for (nat i = 0; i < order.size(); i++) {
			TargetInfo &info = order[i];
			Target *t = target[info.name];
			DEBUG("-- Target " << info.name << " --", NORMAL);
			t->clean();
		}
	}

	bool Project::compile() {
		nat threads = to<nat>(config.getStr("maxThreads", "1"));

		// Force serial execution?
		if (!config.getBool("parallel", true))
			threads = 1;

		if (threads <= 1) {
			return compileST();
		} else {
			return compileMT(threads);
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

		TargetInfo &info = order[id];
		Target *t = target[info.name];

		String banner;
		if (debugLevel >= dbg_NORMAL)
			banner = "-- Target " + info.name + " --";
		SetBanner w(banner.c_str());
		SetPrefix z(prefix.c_str());

		vector<Path> d = dependencies(info.name, info);
		d = removeDuplicates(d);
		for (nat i = 0; i < d.size(); i++) {
			t->addLib(d[i]);
		}

		if (!t->compile()) {
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
		MTState state(*this);

		Thread *threads = new Thread[threadCount];
		for (nat i = 0; i < threadCount; i++) {
			threads[i].start(&MTState::start, state);
		}

		// Join all threads, and read result.
		for (nat i = 0; i < threadCount; i++) {
			threads[i].join();
		}

		return state.ok;
	}

	Project::MTState::MTState(Project &p) :
		p(&p), next(0), ok(true) {

		for (nat i = 0; i < p.order.size(); i++)
			targetDone.insert(make_pair(p.order[i].name, new Condition()));
	}

	Project::MTState::~MTState() {
		for (map<String, Condition *>::iterator i = targetDone.begin(), end = targetDone.end(); i != end; ++i) {
			delete i->second;
		}
	}

	void Project::MTState::start() {
		if (!p->threadMain(*this)) {
			if (!ok)
				return;

			// This does not matter too much if we do it more than once.
			ok = false;

			// Signal all condition variables to cause threads to exit.
			for (map<String, Condition *>::iterator i = targetDone.begin(), end = targetDone.end(); i != end; ++i) {
				i->second->signal();
			}
		}
	}

	bool Project::threadMain(MTState &state) {
		while (true) {
			nat work = atomicInc(state.next);

			// Done?
			if (work >= order.size())
				break;

			// Wait until all dependencies are satisfied.
			TargetInfo &info = order[work];
			for (set<String>::const_iterator i = info.dependsOn.begin(), end = info.dependsOn.end(); i != end; ++i) {
				map<String, Condition *>::iterator f = state.targetDone.find(*i);

				// Note: some dependencies are ignored. That is fine!
				if (f == state.targetDone.end())
					continue;

				f->second->wait();
			}

			// Double-check so that something did not fail.
			if (!state.ok)
				return false;

			if (!compileOne(work, true))
				return false;

			map<String, Condition *>::iterator f = state.targetDone.find(info.name);
			if (f == state.targetDone.end())
				return false;
			f->second->signal();

			// Once again, something failed?
			if (!state.ok)
				return false;
		}

		return true;
	}

	int Project::execute(const vector<String> &params) {
		if (mainTarget.empty()) {
			PLN("Nothing to run!");
			return 1;
		}

		return target[mainTarget]->execute(params);
	}


	void Project::addTargets(const vector<String> &params, TargetQueue &to) {
		for (nat i = 0; i < params.size(); i++)
			addTarget(params[i], to);
	}

	void Project::addTarget(const String &param, TargetQueue &to) {
		Path rel = Path(param);
		if (rel.isAbsolute())
			rel = rel.makeRelative(wd);
		if (rel.isEmpty())
			return;

		to << rel.first();
	}

	Target *Project::loadTarget(const String &name) const {
		Path dir = wd + name;
		dir.makeDir();

		vector<String> vOptions = buildConfig.getArray(name);
		set<String> options(vOptions.begin(), vOptions.end());

		vOptions = buildConfig.getArray("all");
		options.insert(vOptions.begin(), vOptions.end());

		DEBUG("Options for " << name << ": " << join(options), INFO);

		MakeConfig config;

		Path configFile = dir + localConfig;
		if (configFile.exists()) {
			DEBUG("Found local config: " << configFile, INFO);
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

		projectFile.apply(options, opt);
		config.apply(options, opt);

		DEBUG("Configuration for " << name << ": " << opt, VERBOSE);

		return new Target(dir, opt);
	}

}
