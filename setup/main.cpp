#include "../setuputils/pipe_process.h"
#include "../setuputils/find_vs.h"
#include <iostream>
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include <Shlwapi.h>
#include <direct.h>

using std::string;
using std::cout;
using std::endl;
using std::setw;
using std::map;
using std::vector;
using std::istream;
using std::ostream;
using std::stringstream;
using std::istringstream;
using std::ostringstream;

#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "user32.lib")
#pragma warning (disable : 4996) // For getenv.

vector<string> getPath() {
	istringstream pathVar(getenv("PATH"));
	string var;
	vector<string> r;
	while (getline(pathVar, var, ';')) {
		r.push_back(var);
	}
	return r;
}

void moveFile(const string &from, const string &to) {
	if (PathFileExists(to.c_str()))
		DeleteFile(to.c_str());
	MoveFile(from.c_str(), to.c_str());
}

void addPath(const string &to) {
	HKEY envKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &envKey)) {
		cout << "Failed accessing the windows registry." << endl;
		return;
	}

	DWORD type = 0;
	DWORD size = 0;
	if (RegGetValue(envKey, NULL, "Path", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, &type, NULL, &size)) {
		cout << "Failed to open the 'Path' key." << endl;
		return;
	}

	char *data = new char[size * 2]; // Some margin if another process changes the value.
	size = size * 2;
	if (RegGetValue(envKey, NULL, "Path", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, &type, data, &size)) {
		cout << "Failed to open the 'Path' key." << endl;
		return;
	}

	string oldPath = data;
	if (oldPath[oldPath.size() - 1] != ';')
		oldPath += ";";
	oldPath += to;

	if (RegSetValueEx(envKey, "Path", 0, type, (const BYTE *)oldPath.c_str(), oldPath.size() + 1)) {
		cout << "Failed to write the 'Path' key." << endl;
		return;
	}

	RegCloseKey(envKey);

	// Update the ENV in any open shell windows.
	DWORD_PTR result;
	SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 1, (LPARAM)"Environment", SMTO_ABORTIFHUNG | SMTO_BLOCK, 1000, &result);

	cout << "Done! Please restart your terminal windows." << endl;
}

int main(int argc, const char *argv[]) {
	string file;
	if (argc == 1) {
		file = find_vs::find(NULL);
	} else {
		string s(argv[1]);
		file = find_vs::find(&s);
	}

	if (file.empty())
		return 1;

	cout << "Compiling mymake..." << endl;

	if (!PathFileExists("build"))
		CreateDirectory("build", NULL);
	if (!PathFileExists("bin"))
		CreateDirectory("bin", NULL);

	stringstream commands;
	commands << "cl /nologo /EHsc textinc\\*.cpp /Fobuild\\ /Febin\\textinc.exe" << endl;
	commands << "bin\\textinc.exe bin\\templates.h";
	{
		WIN32_FIND_DATA file;
		HANDLE f = FindFirstFile("templates\\*.txt", &file);
		if (f != INVALID_HANDLE_VALUE) {
			do {
				if (file.cFileName[0] != '.')
					commands << " templates\\" << file.cFileName;
			} while (FindNextFile(f, &file) == TRUE);
			FindClose(f);
		}
	}
	commands << endl;
	commands << "cl /nologo /O2 /EHsc /Isrc\\ src\\*.cpp src\\setup\\*.cpp /Fobuild\\ /Femm.exe" << endl;

	stringstream out;
	pipe::runCmd(true, "\"\"" + file + "\"\" x86", commands, out);

	if (!PathFileExists("mm.exe")) {
		cout << "Compilation failed." << endl;
		return 1;
	}

	cout << "Done!" << endl;

	char tmp[MAX_PATH + 1];
	_getcwd(tmp, MAX_PATH + 1);
	string cwd = tmp;
	if (cwd[cwd.size() - 1] != '\\')
		cwd += "\\";

	vector<string> paths = getPath();
	cout << "Do you wish to add mm.exe to your path?" << endl;
	cout << " 0> No, don't do anything." << endl;
	cout << " 1> Create a new entry for the current location (" << cwd <<  "release)" << endl;
	for (size_t i = 0; i < paths.size(); i++)
		cout << setw(2) << (i + 2) << "> Copy mymake to: " << paths[i] << endl;

	size_t id = 0;
	while (true) {
		cout << " ?> ";
		string selection;
		if (!getline(std::cin, selection))
			return 1;

		istringstream iss(selection);
		if (!(iss >> id)) {
			cout << "Not a number, try again!" << endl;
			continue;
		}

		if (id < paths.size() + 2)
			break;

		cout << "Not an option, try again!" << endl;
	}

	if (id == 0) {
		// Don't do anything.
	} else if (id == 1) {
		// Add a new thing to PATH.
		if (!PathFileExists("release"))
			CreateDirectory("release", NULL);
		moveFile("mm.exe", "release\\mm.exe");

		addPath(cwd + "release");
	} else {
		// Move the binary to the proper location.
		moveFile("mm.exe", paths[id - 2] + "\\mm.exe");
	}

	return 0;
}
