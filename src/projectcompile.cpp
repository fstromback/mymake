#include "std.h"
#include "projectcompile.h"

namespace compile {

	set<String> createSet(const String &member) {
		set<String> s;
		s << member;
		return s;
	}

	Project::Project(const Path &wd, const MakeConfig &projectFile, const Config &config) :
		wd(wd),
		projectFile(projectFile),
		config(config) {

		projectFile.apply(createSet("deps"), depsConfig);
		projectFile.apply(createSet("build"), buildConfig);
	}

	Project::~Project() {
		for (map<String, Target *>::iterator i = target.begin(); i != target.end(); ++i) {
			delete i->second;
		}
	}

	bool Project::find() {
		// Figure out which projects we need.

		// TODO? Detect circular depencies?
		TargetQueue q;
		addTargets(config.getArray("input"), q);

		while (q.any()) {
			String now = q.pop();
			DEBUG("Examining target " << now, INFO);

			Target *target = loadTarget(now);
			this->target[now] = target;
			if (!target->find()) {
				DEBUG("Compilation of " << now << " failed!", NORMAL);
				return false;
			}

			TargetInfo info = { now };

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

		return true;
	}

	bool Project::compile() {
		for (nat i = order.size(); i > 0; i--) {
			TargetInfo &info = order[i - 1];
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
		if (order.empty())
			return 0;

		return target[order.front().name]->execute(params);
	}


	void Project::addTargets(const vector<String> &params, TargetQueue &to) {
		for (nat i = 0; i < params.size(); i++)
			addTarget(params[i], to);
	}

	void Project::addTarget(const String &param, TargetQueue &to) {
		Path rel = Path(param).makeAbsolute().makeRelative(wd);
		if (rel.isEmpty())
			return;

		to << rel.first();
	}

	Target *Project::loadTarget(const String &name) const {
		Path dir = wd + name;
		dir.makeDir();

		vector<String> vOptions = buildConfig.getArray(name);
		set<String> options(vOptions.begin(), vOptions.end());

		MakeConfig config;

		Path configFile = dir + localConfig;
		if (configFile.exists()) {
			DEBUG("Found local config: " << configFile, INFO);
			config.load(configFile);
		}

		Config opt;
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

		DEBUG("Configuration options for " << name << ": " << opt, VERBOSE);

		return new Target(dir, opt);
	}

}