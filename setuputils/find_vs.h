#pragma once

#ifdef _MSC_VER

#include <string>
#include <vector>
#include <sstream>
#include <Windows.h>

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*x))
#endif

#pragma comment (lib, "shlwapi.lib")

namespace find_vs {

	using std::vector;
	using std::string;
	using std::istringstream;
	using std::cout;
	using std::endl;

	/**
	 * Utility functions for finding Visual Studio on Windows.
	 *
	 * Include from one cpp-file for each project.
	 */

	// What to look for: It seems "vcvarsall.bat" is always present and works approximately the same across versions.
	const char *toolNames[] = {
		// "VsDevCmd.bat",  // At least on VS2019
		"vcvarsall.bat", // At least on VS2008
	};

	bool isToolFile(const char *file) {
		for (size_t i = 0; i < ARRAY_COUNT(toolNames); i++) {
			if (_stricmp(file, toolNames[i]) == 0)
				return true;
		}
		return false;
	}

	// Find vs files.
	void find(vector<string> &out, const string &root) {
		WIN32_FIND_DATA file;
		HANDLE handle = FindFirstFile((root + "*").c_str(), &file);
		if (handle == INVALID_HANDLE_VALUE)
			return;

		do {
			if (strcmp(file.cFileName, ".") == 0 || strcmp(file.cFileName, "..") == 0) {
				continue;
			} else if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				find(out, root + file.cFileName + "\\");
			} else if (isToolFile(file.cFileName)) {
				out.push_back(root + file.cFileName);
			}
		} while (FindNextFile(handle, &file) == TRUE);

		FindClose(handle);
	}

	// Pick one out of multiple candidates.
	string pick(const vector<string> &candidates) {
		if (candidates.size() == 1)
			return candidates[0];

		cout << "Multiple versions of Visual Studio were found. Please pick the one you want to use:" << endl;
		for (size_t i = 0; i < candidates.size(); i++)
			cout << (i + 1) << "> " << candidates[i] << endl;

		while (true) {
			cout << "?> ";
			string selection;
			if (!getline(std::cin, selection))
				exit(1);

			istringstream iss(selection);
			size_t id;
			if (!(iss >> id)) {
				cout << "Not a number, try again!" << endl;
				continue;
			}

			if (id > 0 && id <= candidates.size())
				return candidates[id - 1];

			cout << "Not an option, try again!" << endl;
		}
	}

	// Find vs using default heuristics. Provide a hint if desired.
	string find(const string *hint) {
		vector<string> candidates;
		if (hint) {
			DWORD attrs = GetFileAttributes(hint->c_str());
			if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				return *hint;
			} else {
				string name = *hint;
				if (name[name.size() - 1] != '\\')
					name += "\\";
				cout << "Looking for the Visual Studio command line tools in " << name << "..." << endl;
				find(candidates, name);
			}
		} else {
			cout << "Looking for the Visual Studio command line tools..." << endl;
			find(candidates, "C:\\Program Files (x86)\\");
			find(candidates, "C:\\Program Files\\");
		}

		if (candidates.empty()) {
			cout << "Unable to find the Visual Studio command line tools using any of the following names:" << endl;
			for (size_t i = 0; i < ARRAY_COUNT(toolNames); i++)
				cout << "  " << toolNames[i] << endl;
			cout << "Please try specifying either the path to your Visual Studio installation, or the full path to any of the above files." << endl;
			return "";
		}

		return pick(candidates);
	}

}

#endif WINDOWS
