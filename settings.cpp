#include <iostream>
#include <fstream>

#include "settings.h"

using namespace std;

Settings::Settings() {
  loadFile("~/.mymake");
  loadFile(".mymake");
}

Settings::~Settings() {}

void Settings::parseArguments(int argc, char **argv) {
  executable = argv[0];

  if (argc <= 1) return;

  string identifier = "input";

  for (int i = 1; i < argc; i ++) {
    string arg = argv[i];

    if (arg != "") {
      if (arg[0] == '-') {
	if (arg == "-o") {
	  identifier = "out";
	} else if (arg == "-e") {
	  executeCompiled = true;
	} else if (arg == "-ne") {
	  executeCompiled = false;
	}
      } else {
	storeItem(identifier, arg);
	identifier = "input";
      }
    }
  }
}


void Settings::loadFile(const string &file) {
  ifstream f(file.c_str());

  if (f.fail()) return;

  while (!f.eof()) {
    string line;
    getline(f, line);
    
    if (line.size() > 0) {
      if (line[0] != '#') {
	parseLine(line);
      }
    }
  }
}

void Settings::parseLine(const string &line) {
  int eq = line.find('=');
  if (eq == string::npos) return;

  storeItem(line.substr(0, eq), line.substr(eq + 1));
}

void Settings::storeItem(const string &identifier, const string &value) {
  if (identifier == "input") {
    inputFiles.push_back(value);
  } else if (identifier == "ext") {
    cppExtensions.push_back(value);
  } else if (identifier == "out") {
    outFile = value;
  } else if (identifier == "compile") {
    compile = value;
  } else if (identifier == "link") {
    link = value;
  } else if (identifier == "build") {
    buildPath = value;
  } else if (identifier == "execute") {
    executeCompiled = (value == "yes");
  }
}

void Settings::outputUsage() const {
  cout << "Usage:" << endl;
  cout << executable << " <file> [-o <output>] [-ne] [-e]" << endl << endl;
  cout << "file   : The root source file to compile (may contain multiple files)." << endl;
  cout << "output : The name of the executable file to be created." << endl;
  cout << "-e     : Execute the compiled file on success." << endl;
  cout << "-ne    : Do not execute the compiled file on success." << endl;
}

string Settings::replace(const string &in, const string &find, const string &replace) const {
  string copy = in;
  int pos = in.find(find);
  if (pos == string::npos) return in;

  return copy.replace(pos, find.length(), replace);
}

string Settings::getCompileCommand(const string &file, const string &output) const {
  string result = replace(replace(compile, "<file>", file), "<output>", output);
  return result;
}

string Settings::getLinkCommand(const string &files) const {
  string result = replace(replace(link, "<output>", outFile), "<files>", files);
  return result;
}

bool Settings::enoughForCompilation() const {
  if (inputFiles.size() == 0) return false;
  if (cppExtensions.size() == 0) return false;
  if (buildPath.size() == 0) return false;
  if (outFile.size() == 0) return false;

  return true;
}
