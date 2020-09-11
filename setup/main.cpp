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
using std::wstring;
using std::wcout;
using std::endl;
using std::vector;

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*x))

// What to look for: It seems "vcvarsall.bat" is always present and works approximately the same across versions.
const wchar_t *toolNames[] = {
	// L"VsDevCmd.bat",  // At least on VS2019
	L"vcvarsall.bat", // At least on VS2008
};

bool isToolFile(const wchar_t *file) {
	for (size_t i = 0; i < ARRAY_COUNT(toolNames); i++) {
		if (_wcsicmp(file, toolNames[i]) == 0)
			return true;
	}
	return false;
}

void findFiles(vector<wstring> &out, const wstring &root) {
	WIN32_FIND_DATA file;
	HANDLE handle = FindFirstFile((root + L"*").c_str(), &file);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	do {
		if (wcscmp(file.cFileName, L".") == 0 || wcscmp(file.cFileName, L"..") == 0) {
			continue;
		} else if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			findFiles(out, root + file.cFileName + L"\\");
		} else if (isToolFile(file.cFileName)) {
			out.push_back(root + file.cFileName);
		}
	} while (FindNextFile(handle, &file) == TRUE);

	FindClose(handle);
}

wstring pickFile(const vector<wstring> &candidates) {
	if (candidates.size() == 1)
		return candidates[0];

	wcout << L"Multiple versions of Visual Studio were found. Please pick the one you want to use:" << endl;
	for (size_t i = 0; i < candidates.size(); i++)
		wcout << (i + 1) << L"> " << candidates[i] << endl;

	while (true) {
		wcout << "?> ";
		wstring selection;
		if (!getline(std::wcin, selection))
			exit(1);

		std::wistringstream iss(selection);
		size_t id;
		if (!(iss >> id)) {
			wcout << "Not a number, try again!" << endl;
			continue;
		}

		if (id > 0 && id <= candidates.size())
			return candidates[id - 1];

		wcout << "Not an option, try again!" << endl;
	}
}


int main(int argc, const wchar_t *argv[]) {
	vector<wstring> candidates;

	if (argc == 1) {
		wcout << L"Looking for the Visual Studio command line tools..." << endl;
		findFiles(candidates, L"C:\\Program Files (x86)\\");
		findFiles(candidates, L"C:\\Program Files\\");
	} else if (argc == 2) {
		DWORD attrs = GetFileAttributes(argv[1]);
		if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			// It's the bat file we're supposedly looking for!
			candidates.push_back(argv[1]);
		} else {
			wstring name = argv[1];
			if (name[name.size() - 1] != '\\')
				name += L"\\";
			wcout << L"Looking for the Visual Studio command line tools in " << name << L"..." << endl;
			findFiles(candidates, name);
		}
	}

	if (candidates.empty()) {
		wcout << L"Unable to find the Visual Studio command line tools using any of the following names:" << endl;
		for (size_t i = 0; i < ARRAY_COUNT(toolNames); i++)
			wcout << L"  " << toolNames[i] << endl;
		wcout << L"Please try specifying either the path to your Visual Studio installation, or the full path to any of the above files." << endl;
		return 1;
	}

	wstring file = pickFile(candidates);
	wstring cmd = _wgetenv(L"comspec");

	std::wcout << L"Examining environment variables..." << std::endl;

	std::istringstream in("set\n");
	std::ostringstream out;
	runProcess(L"C:\\Windows\\system32\\cmd.exe", L"/K \"\"" + file + L"\" amd64\"", in, out);
	runProcess(L"C:\\Windows\\system32\\cmd.exe", L"/K \"\"" + file + L"\" x86\"", in, out);
	runProcess(L"C:\\Windows\\system32\\cmd.exe", L"", in, out);

	std::cout << out.str() << std::endl;

	return 0;
}
