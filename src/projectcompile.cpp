#include "std.h"
#include "projectcompile.h"

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
			this->target[now] = target;
			if (!target->find()) {
				DEBUG("Compilation of " << now << " failed!", NORMAL);
				return false;
			}

			TargetInfo info = { now, set<String>() };

			// Add any dependent projects.
			for (set<String>::const_iterator i = target->dependsOn.begin(); i != target->dependsOn.end(); ++i) {
				q << *i;
				info.dependsOn << *i;
			}

			// Add any explicit dependent projects.
			vector<String> depends = depsConfig.getArray(now);
			for (nat i = 0; i < depends.size(); i++) {
				q << depends[i];
				info.dependsOn << depends[i];
			}

			order << info;
		}

		if (order.empty()) {
			DEBUG("No input files or projects. Compilation failed.", NORMAL);
			return false;
		}

		try {
			order = topoSort(order);
		} catch (const TopoError &e) {
			PLN("Error: " << e.what());
		}

		return true;
	}

	bool Project::compile() {
		for (nat i = 0; i < order.size(); i++) {
			TargetInfo &info = order[i];
			Target *t = target[info.name];
			DEBUG("-- Target " << info.name << " --", NORMAL);

			for (set<String>::const_iterator i = info.dependsOn.begin(); i != info.dependsOn.end(); ++i) {
				Target *z = target[*i];
				if (z->linkOutput) {
					DEBUG(info.name << " includes dependent " << z->output, INFO);
					t->addLib(z->output);
				}
			}

			if (!t->compile()) {
				DEBUG("Compilation of " << info.name << " failed!", NORMAL);
				return false;
			}
		}

		return true;
	}

	int Project::execute(const vector<String> &params) {
		if (mainTarget.empty()) {
			PLN("Nothing to run!");
			return 1;
		}

		return target[order.front().name]->execute(params);
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
