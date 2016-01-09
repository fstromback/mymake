#include <iostream>
#include <fstream>
#include <string>

using namespace std;

string name(string file) {
	size_t lastSlash = file.rfind('/');
	if (lastSlash == string::npos)
		lastSlash = file.rfind('\\');
	if (lastSlash != string::npos)
		file = file.substr(lastSlash + 1);

	size_t lastDot = file.rfind('.');
	if (lastDot == string::npos)
		return file;
	return file.substr(0, lastDot);
}

int main(int argc, const char *argv[]) {
	if (argc < 3) {
		cout << "Usage: " << argv[0] << " <output> <input> ..." << endl;
		return 1;
	}

	ofstream cpp(argv[1]);
	if (!cpp) {
		cout << "Failed to open " << argv[1] << endl;
		return 1;
	}

	cpp << "#pragma once" << endl;

	for (int i = 2; i < argc; i++) {
		ifstream src(argv[i]);
		if (!src) {
			cout << "Failed to open " << argv[i] << endl;
			return 1;
		}

		cpp << "const char *" << name(argv[i]) << " =" << endl;

		bool first = true;
		string line;
		while (getline(src, line)) {
			if (!first)
				cpp << endl;
			cpp << "\t\"";

			for (size_t i = 0; i < line.size(); i++) {
				switch (line[i]) {
				case '\t':
					cpp << "\\t";
					break;
				case '\\':
					cpp << "\\\\";
					break;
				case '"':
					cpp << "\\\"";
					break;
				default:
					cpp << line[i];
					break;
				}
			}

			cpp << "\\n\"";
			first = false;
		}

		cpp << ';' << endl;
	}

	return 0;
}
