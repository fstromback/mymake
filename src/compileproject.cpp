#include "std.h"
#include "compileproject.h"

namespace compile {

	Project::Project(const Path &wd, const MakeConfig &projectFile, const Config &config) :
		wd(wd),
		projectFile(projectFile),
		config(config) {}

	Project::~Project() {
		for (nat i = 0; i < targets.size(); i++)
			delete targets[i];
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
			targets << target;
			if (!target->find()) {
				DEBUG("Compilation of " << now << " failed!", NORMAL);
				return false;
			}

			// Add any dependent projects.
			for (set<String>::const_iterator i = target->dependsOn.begin(); i != target->dependsOn.end(); ++i) {
				q << *i;
			}
		}

		return true;
	}

	bool Project::compile() {
		return false;
	}

	int Project::execute(const vector<String> &params) {
		return 0;
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

		vector<String> vOptions = config.getArray(name);
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
