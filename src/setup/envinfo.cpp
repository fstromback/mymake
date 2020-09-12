#include "std.h"
#include "envinfo.h"
#include "env.h"
#include "../../setuputils/find_vs.h"
#include "../../setuputils/pipe_process.h"

namespace setup {

#ifdef WINDOWS

#pragma warning (disable: 4996) // for getenv()

	EnvVars captureEnv(const String &command) {
		String cmd = getenv("comspec");
		istringstream in("set\n");
		std::stringstream out;
		pipe::runProcess(cmd, "/K " + command, in, out);

		return parseEnv(out);
	}

	void outputEnv(ostream &to, const EnvVars &vars) {
		for (EnvVars::const_iterator i = vars.begin(); i != vars.end(); ++i) {
			for (size_t j = i->second.size(); j > 0; j--) {
				if (!i->second[j - 1].empty()) {
					to << "env+=" << i->first << "<=" << i->second[j - 1] << endl;
				}
			}
		}
	}

	bool supportsFlag(const String &file, const String &flag) {
		String cmd = getenv("comspec");
		istringstream in("cl " + flag + "\n");
		ostringstream out;
		pipe::runProcess(cmd, "/K \"\"" + file + "\"\" x86", in, out);

		size_t first = out.str().find(flag);
		size_t second = out.str().find(flag, first + 1);
		bool ok = second == String::npos;
		std::cout << "-> " << flag << " supported: " << (ok ? "yes" : "no") << endl;
		return ok;
	}

	String envinfo(const String *param) {
		String file = find_vs::find(param);
		if (file.empty()) {
			std::cout << "WARNING: Failed to find Visual Studio, will not setup environvent variables." << endl;
			return "";
		}

		EnvVars plain = captureEnv("");
		EnvVars x86 = captureEnv("\"\"" + file + "\"\" x86");
		EnvVars x64 = captureEnv("\"\"" + file + "\"\" amd64");

		x86 = subtract(x86, plain);
		x64 = subtract(x64, plain);

		std::cout << "Examining supported features..." << endl;

		// Faster PCB file generation when multiple instances. From VS 2017.
		bool mtPdb = supportsFlag(file, "/Zf");

		// Flag to support multiple processes. Required from VS 2010.
		bool mtFlag = supportsFlag(file, "/FS");

		// Supports setting standard?
		bool stdFlag = supportsFlag(file, "/std:c++14");

		ostringstream config;
		config << "[windows,!64]" << endl;
		config << "#Automatically extracted from " << file << endl;
		outputEnv(config, x86);

		config << "[windows,64]" << endl;
		config << "#Automatically extracted from " << file << endl;
		outputEnv(config, x64);

		config << endl;
		if (mtPdb | mtFlag | stdFlag) {
			ostringstream flags;
			if (mtPdb)
				flags << " /Zf";
			if (mtFlag)
				flags << " /FS" << endl;
			if (stdFlag)
				flags << " <useStandard*standard>";

			config << "[windows]" << endl;
			config << "flags+=" << flags.str().substr(1) << endl;
		}

		return config.str();
	}

#else

	String envinfo(const String *params) {
		return "";
	}

#endif
}
