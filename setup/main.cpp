#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <Windows.h>
#include "process.h"

#pragma comment (lib, "shlwapi.lib")
#pragma warning (disable: 4996) // Unsafe C-functions, _wgetenv in this case.

using std::string;
using std::cout;
using std::endl;
using std::vector;

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*x))

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

void findFiles(vector<string> &out, const string &root) {
	WIN32_FIND_DATA file;
	HANDLE handle = FindFirstFile((root + "*").c_str(), &file);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	do {
		if (strcmp(file.cFileName, ".") == 0 || strcmp(file.cFileName, "..") == 0) {
			continue;
		} else if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			findFiles(out, root + file.cFileName + "\\");
		} else if (isToolFile(file.cFileName)) {
			out.push_back(root + file.cFileName);
		}
	} while (FindNextFile(handle, &file) == TRUE);

	FindClose(handle);
}

string pickFile(const vector<string> &candidates) {
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

		std::istringstream iss(selection);
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


int main(int argc, const char *argv[]) {
	vector<string> candidates;

	if (argc == 1) {
		cout << "Looking for the Visual Studio command line tools..." << endl;
		findFiles(candidates, "C:\\Program Files (x86)\\");
		findFiles(candidates, "C:\\Program Files\\");
	} else if (argc == 2) {
		DWORD attrs = GetFileAttributes(argv[1]);
		if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			// It's the bat file we're supposedly looking for!
			candidates.push_back(argv[1]);
		} else {
			string name = argv[1];
			if (name[name.size() - 1] != '\\')
				name += "\\";
			cout << "Looking for the Visual Studio command line tools in " << name << "..." << endl;
			findFiles(candidates, name);
		}
	}

	if (candidates.empty()) {
		cout << "Unable to find the Visual Studio command line tools using any of the following names:" << endl;
		for (size_t i = 0; i < ARRAY_COUNT(toolNames); i++)
			cout << "  " << toolNames[i] << endl;
		cout << "Please try specifying either the path to your Visual Studio installation, or the full path to any of the above files." << endl;
		return 1;
	}

	string file = pickFile(candidates);
	string cmd = getenv("comspec");

	cout << "Examining environment variables..." << endl;

	std::istringstream in("set\n");
	std::ostringstream out;
	runProcess("C:\\Windows\\system32\\cmd.exe", "/K \"\"" + file + "\" amd64\"", in, out);
	in = std::istringstream("set\n");
	runProcess("C:\\Windows\\system32\\cmd.exe", "/K \"\"" + file + "\" x86\"", in, out);
	in = std::istringstream("set\n");
	runProcess("C:\\Windows\\system32\\cmd.exe", "", in, out);

	std::cout << out.str() << std::endl;

	return 0;
}
