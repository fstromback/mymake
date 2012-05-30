#include <iostream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"
#include "utils.h"

using namespace std;

Settings::Settings() {

  forceRecompilation = false;
  executeCompiled = false;
  showSettings = false;
  showHelp = false;
  debugOutput = false;

  loadFile(getHomeFile(".mymake"));
  loadFile(".mymake");
}

Settings::~Settings() {}

string Settings::getHomeFile(const string &file) const {
  char *home = getenv("HOME");
  if (home != 0) {
    string h = home;
    if (h[h.size() - 1] == '/') {
      return h + file;
    } else {
      return h + "/" + file;
    }
  }
}

void Settings::install() const {
  ofstream out(getHomeFile(".mymake").c_str());

  out << "#Configuration for mymake" << endl;
  out << "#Lines beginning with a # are treated as comments." << endl;
  out << "#Lines containing errors are ignored." << endl;
  out << "" << endl;
  out << "#These are the general settings, which can be overriden in a local .mymake-file" << endl;
  out << "#or as command-line arguments to the executable in some cases." << endl;
  out << "" << endl;
  out << "#The file-types of the implementation files. Used to match header-files to implementation-files." << endl;
  out << "ext=cpp" << endl;
  out << "ext=cc" << endl;
  out << "ext=cxx" << endl;
  out << "ext=c++" << endl;
  out << "" << endl;
  out << "#The extension for executable files on the system." << endl;
  out << "executableExt=" << endl;
  out << "" << endl;
  out << "#Input file(s)" << endl;
  out << "#input=<filename>" << endl;
  out << "" << endl;
  out << "#Output (defaults to the name of the first input file)" << endl;
  out << "#out=<filename>" << endl;
  out << "" << endl;
  out << "#Command used to compile a single source file into a unlinked file." << endl;
  out << "#<file> will be replaced by the input file and" << endl;
  out << "#<output> will be replaced by the output file" << endl;
  out << "compile=g++ <file> -c -o <output>" << endl;
  out << "" << endl;
  out << "#Command to link the intermediate files into an executable." << endl;
  out << "#<files> is the list of intermediate files and" << endl;
  out << "#<output> is the name of the final executable to be created." << endl;
  out << "link=g++ <files> -o <output>" << endl;
  out << "" << endl;
  out << "#The directory to be used for intermediate files" << endl;
  out << "build=build/" << endl;
  out << "" << endl;
  out << "#Execute the compiled file after successful compilation?" << endl;
  out << "execute=yes" << endl;
}

void Settings::parseArguments(int argc, char **argv) {
  executable = File(argv[0]).getTitle();

  if (argc <= 1) return;

  string identifier = "input";
  showSettings = debugOutput;

  for (int i = 1; i < argc; i ++) {
    string arg = argv[i];

    if (arg != "") {
      if (arg == "-?") {
	showHelp = true;
      } else if (arg == "-o") {
	identifier = "out";
      } else if (arg == "-f") {
	forceRecompilation = true;
      } else if (arg == "-e") {
	executeCompiled = true;
      } else if (arg == "-ne") {
	executeCompiled = false;
      } else if (arg == "-clean") {
	clean = true;
      } else if (arg == "-config") {
	doInstall = true;
	return;
      } else if (arg == "-s") {
	showSettings = true;
      } else if (arg == "-debug"){
	debugOutput = true;
	showSettings = true;
      } else if (arg == "-a") {
	executeCompiled = true;
	addProcessParameters(argc, argv, i + 1);
	break;
      } else {
	storeItem(identifier, arg);
	identifier = "input";
      }
    }
  }

  if (showSettings) outputConfig();
}

void Settings::addProcessParameters(int argc, char **argv, int i) {
  for (; i < argc; i++) {
    commandLineParams.push_back(string(argv[i]));
  }
}

char **Settings::getExecParams() const {
  char **arglist = new char*[commandLineParams.size() + 2];
  
  arglist[0] = new char[active.outFile.size() + 1];
  strcpy(arglist[0], active.outFile.c_str());
  int pos = 1;
  for (list<string>::const_iterator i = commandLineParams.begin(); i != commandLineParams.end(); i++) {
    arglist[pos] = new char[i->size() + 1];
    strcpy(arglist[pos], i->c_str());
    pos++;
  }
  arglist[pos] = 0;

  return arglist;
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
    addSingle(inputFiles, value);
  } else if (identifier == "ext") {
    addSingle(cppExtensions, value);
  } else if (identifier == "executableExt") {
    executableExt = value;
  } else if (identifier == "out") {
    active.outFile = value;
  } else if (identifier == "compile") {
    active.compile = value;
  } else if (identifier == "link") {
    active.link = value;
  } else if (identifier == "build") {
    active.buildPath = value;
  } else if (identifier == "execute") {
    executeCompiled = (value == "yes");
  } else if (identifier == "debugOutput") {
    debugOutput = (value == "yes");
    if (debugOutput) showSettings = true;
  }
}

void Settings::outputUsage() const {
  cout << "Usage:" << endl;
  cout << executable << " <file> [-o <output>] [-ne] [-e] [-f] [-a <arg1> ... <argn>] [-clean] [-s]" << endl;
  cout << executable << " -config" << endl;
  cout << endl;
  cout << "file    : The root source file to compile (may contain multiple files)." << endl;
  cout << "output  : The name of the executable file to be created." << endl;
  cout << "-e      : Execute the compiled file on success." << endl;
  cout << "-ne     : Do not execute the compiled file on success." << endl;
  cout << "-f      : Force recompilation." << endl;
  cout << "-a      : Arguments to the started process (sets -e as well)." << endl;
  cout << "-s      : Show settings before compilation." << endl;
  cout << "-clean  : Remove all intermediate files." << endl;
  cout << "-config : Setup the .mymake-file in the home directory." << endl;
}

string Settings::replace(const string &in, const string &find, const string &replace) const {
  string copy = in;
  int pos = in.find(find);
  if (pos == string::npos) return in;

  return copy.replace(pos, find.length(), replace);
}

string Settings::getCompileCommand(const string &file, const string &output) const {
  string result = replace(replace(active.compile, "<file>", file), "<output>", output);
  return result;
}

string Settings::getLinkCommand(const string &files) const {
  string result = replace(replace(active.link, "<output>", quote(active.outFile)), "<files>", files);
  return result;
}

bool Settings::enoughForCompilation() {
  if (!clean) if (inputFiles.size() == 0) return false;
  if (cppExtensions.size() == 0) return false;
  if (active.buildPath.size() == 0) return false;

  if (active.outFile.size() == 0) {
    File firstInFile(inputFiles.front());
    active.outFile = firstInFile.modifyType(executableExt).getFullPath();
  }

  return true;
}

void Settings::outputConfig() const {
  cout << "Parameters used:" << endl;
  cout << "Input files: " << inputFiles << endl;
  cout << "Extensions: " << cppExtensions << endl;
  cout << "Output file: " << active.outFile.c_str() << endl;
  cout << "Executable extension: " << executableExt.c_str() << endl;
  cout << endl;
  cout << "Build path: " << active.buildPath.c_str() << endl;
  cout << "Compile with: " << active.compile.c_str() << endl;
  cout << "Link with: " << active.link.c_str() << endl;
  cout << endl;
  cout << "Execute file: " << (executeCompiled ? "yes" : "no") << endl;
  cout << "Force recompilation: " << (forceRecompilation ? "yes" : "no") << endl;
  cout << endl;
}
