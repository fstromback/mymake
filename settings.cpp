#include <iostream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include "settings.h"
#include "utils.h"
#include "file.h"

using namespace std;

Settings::Settings() {
  if (windows) {
    intermediateExt = "obj";
    platform = "win";
  } else {
    platform = "unix";
    intermediateExt = "o";
  }

  forceRecompilation = false;
  executeCompiled = false;
  showSettings = false;
  showHelp = false;
  debugOutput = false;
  showTime = false;

  loadFile(getHomeFile(".mymake"));
  loadFile(".mymake");
}

Settings::~Settings() {}

bool Settings::copySettings() const {
  ifstream in(getHomeFile(".mymake").c_str());
  if (!in) return false;
  ofstream out(".mymake");

  out << in.rdbuf();
  return true;
}

void Settings::install() const {
  ofstream out(getHomeFile(".mymake").c_str());

  out << "#Configuration for mymake" << endl;
  out << "#Lines beginning with a # are treated as comments." << endl;
  out << "#Lines containing errors are ignored." << endl;
  out << "#Lines starting with win. or unix. will only be considered during windows and unix compilation." << endl;
  out << "" << endl;
  out << "#These are the general settings, which can be overriden in a local .mymake-file" << endl;
  out << "#or as command-line arguments to the executable in some cases." << endl;
  out << "" << endl;
  out << "#The file-types of the implementation files. Used to match header-files to implementation-files." << endl;
  out << "ext=cpp" << endl;
  out << "ext=cc" << endl;
  out << "ext=cxx" << endl;
  out << "ext=c++" << endl;
  out << "ext=c" << endl;
  out << "" << endl;
  out << "#The extension for executable files on the system." << endl;
  out << "executableExt=" << endl;
  out << "win.executableExt=exe" << endl;
  out << "" << endl;
  out << "#The extension for intermediate files on the system." << endl;
  out << "intermediate=o" << endl;
  out << "win.intermediate=obj" << endl;
  out << "" << endl;
  out << "#Input file(s)" << endl;
  out << "#input=<filename>" << endl;
  out << "" << endl;
  out << "#Ignore specific files (wildcards supported)." << endl;
  out << "#ignore=*.template.*" << endl;
  out << "" << endl;
  out << "#Include paths:" << endl;
  out << "#include=./" << endl;
  out << "" << endl;
  out << "#Include paths command line to compiler" << endl;
  out << "includeCl=-iquote " << endl;
  out << "win.includeCl=/I " << endl;
  out << "" << endl;
  out << "#Command line option to add library" << endl;
  out << "libraryCl=-l" << endl;
  out << "win.libraryCl=" << endl;
  out << "" << endl;
  out << "#Required libraries" << endl;
  out << "#libs=" << endl;
  out << "" << endl;
  out << "#Output (defaults to the name of the first input file)" << endl;
  out << "#out=<filename>" << endl;
  out << "" << endl;
  out << "#Command used to compile a single source file into a unlinked file." << endl;
  out << "#<file> will be replaced by the input file and" << endl;
  out << "#<output> will be replaced by the outputn file" << endl;
  out << "compile=g++ <file> -c <includes> -o <output>" << endl;
  out << "win.compile=cl <file> /nologo /c /EHsc /Fo<output>" << endl;
  out << "" << endl;
  out << "#Command to link the intermediate files into an executable." << endl;
  out << "#<files> is the list of intermediate files and" << endl;
  out << "#<output> is the name of the final executable to be created." << endl;
  out << "#<libs> is the list of library includes generated." << endl;
  out << "link=g++ <libs> <files> -o <output>" << endl;
  out << "win.link=link <libs> <files> /nologo /OUT:<output>" << endl;
  out << "" << endl;
  out << "#The directory to be used for intermediate files" << endl;
  out << "build=build" << PATH_DELIM << endl;
  out << "" << endl;
  out << "#Execute the compiled file after successful compilation?" << endl;
  out << "execute=yes" << endl;
  out << "" << endl;
  out << "#Show compilation time" << endl;
  out << "showTime=no" << endl;
  out << "" << endl;
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
      } else if (arg == "-cp") {
	doCopySettings = true;
	return;
      } else if (arg == "-s") {
	showSettings = true;
      } else if (arg == "-debug"){
	debugOutput = true;
	showSettings = true;
	showTime = true;
      } else if (arg == "-t") {
	showTime = true;
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
  size_t eq = line.find('=');
  if (eq == string::npos) return;

  string key = line.substr(0, eq);
  string value = line.substr(eq + 1);

  size_t dot = key.find('.');
  if (dot != string::npos) {
    string arch = key.substr(0, dot);
    key = key.substr(dot + 1);

    if (arch != platform) return;
  }

  storeItem(key, value);
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
  } else if (identifier == "ignore") {
    ignoreFiles.push_back(value);
  } else if (identifier == "compile") {
    active.compile = value;
  } else if (identifier == "link") {
    active.link = value;
  } else if (identifier == "intermediate") {
    intermediateExt = value;
  } else if (identifier == "build") {
    active.buildPath = value;
  } else if (identifier == "execute") {
    executeCompiled = (value == "yes");
  } else if (identifier == "debugOutput") {
    debugOutput = (value == "yes");
    if (debugOutput) showSettings = true;
    if (debugOutput) showTime = true;
  } else if (identifier == "showTime") {
    showTime = (value == "yes");
  } else if (identifier == "include") {
    includePaths = parseCommaList(value);
  } else if (identifier == "includeCl") {
    includeCommandLine = value;
  } else if (identifier == "libraryCl") {
    libraryCommandLine = value;
  } else if (identifier == "libs") {
    libraries = parseCommaList(value);
  }
}

list<string> Settings::parseCommaList(const string &value) {
  list<string> strs;

  size_t pos = 0;
  while (pos < value.size()) {
    size_t comma = value.find(',', pos);
    if (comma == string::npos) {
      comma = value.size();
    }

    string str = value.substr(pos, comma - pos);
    strs.push_back(str);
    pos = comma + 1;
  }

  return strs;
}

void Settings::outputUsage() const {
  cout << "Usage:" << endl;
  cout << executable << " <file> [-o <output>] [-ne] [-e] [-f] [-clean] [-s] [-t] [-a <arg1> ... <argn>]" << endl;
  cout << executable << " -config" << endl;
  cout << executable << " -cp" << endl;
  cout << endl;
  cout << "file    : The root source file to compile (may contain multiple files)." << endl;
  cout << "output  : The name of the executable file to be created." << endl;
  cout << "-e      : Execute the compiled file on success." << endl;
  cout << "-ne     : Do not execute the compiled file on success." << endl;
  cout << "-f      : Force recompilation." << endl;
  cout << "-a      : Arguments to the started process (sets -e as well)." << endl;
  cout << "-s      : Show settings before compilation." << endl;
  cout << "-t      : Show execution time at end." << endl;
  cout << "-clean  : Remove all intermediate files." << endl;
  cout << "-config : Setup the .mymake-file in the home directory." << endl;
  cout << "-cp     : Copy the system wide .mymake-file to the current directory." << endl;
}

string Settings::replace(const string &in, const string &find, const string &replace) const {
  string copy = in;
  int pos = in.find(find);
  if (pos == string::npos) return in;

  return copy.replace(pos, find.length(), replace);
}

string Settings::getIncludeString() const {
  ostringstream ss;
  bool first = true;

  for (list<string>::const_iterator i = includePaths.begin(); i != includePaths.end(); i++) {
    if (!first) ss << " ";
    ss << includeCommandLine << *i;
    first = false;
  }

  return ss.str();
}

string Settings::getLibString() const {
  ostringstream ss;
  bool first = true;

  for (list<string>::const_iterator i = libraries.begin(); i != libraries.end(); i++) {
    if (!first) ss << " ";
    ss << libraryCommandLine << *i;
    first = false;
  }

  return ss.str();
}

string Settings::getCompileCommand(const string &file, const string &output) const {
  string result = replace(active.compile, "<file>", file);
  result = replace(result, "<output>", output);
  result = replace(result, "<includes>", getIncludeString());
  //string result = replace(replace(active.compile, "<file>", file), "<output>", output);
  return result;
}

string Settings::getLinkCommand(const string &files) const {
  string result = replace(active.link, "<output>", quote(active.outFile));
  result = replace(result, "<files>", files);
  result = replace(result, "<libs>", getLibString());
  // string result = replace(replace(active.link, "<output>", quote(active.outFile)), "<files>", files);
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
  cout << "Ignore: " << ignoreFiles << endl;
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
  cout << "Includes command: " << includeCommandLine << endl;
  cout << "Include paths: ";
  for (list<string>::const_iterator i = includePaths.begin(); i != includePaths.end(); i++) {
    cout << *i << " ";
  }
  cout << endl;
  cout << "Library command: " << libraryCommandLine << endl;
  for (list<string>::const_iterator i = libraries.begin(); i != libraries.end(); i++) {
    cout << *i << " ";
  }
  cout << endl;
}

bool Settings::ignoreFile(const File &file) const {
  if (ignoreFile(file.getTitle())) {
    if (debugOutput) cout << "Ignored file " << file << endl;
    return true;
  } else {
    return false;
  }
}

bool Settings::ignoreFile(const string &title) const {
  for (list<Wildcard>::const_iterator i = ignoreFiles.begin(); i != ignoreFiles.end(); i++) {
    if (i->matches(title)) return true;
  }
  return false;
}
