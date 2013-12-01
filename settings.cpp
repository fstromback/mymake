#include <iostream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <algorithm>

#include "settings.h"
#include "utils.h"
#include "file.h"
#include "globals.h"

using namespace std;

Settings::Settings() {
  if (windows) {
    intermediateExt = "obj";
    platforms.push_back("win");
  } else {
    platforms.push_back("unix");
    intermediateExt = "o";
  }

  forceRecompilation = false;
  executeCompiled = false;
  showHelp = false;
  debugLevel = NONE;
  showTime = false;
}

Settings::~Settings() {}

void Settings::loadSettings() {
  loadFile(getHomeFile(".mymake"));
  loadFile(".mymake");
}

bool Settings::copySettings() const {
  ifstream in(getHomeFile(".mymake").c_str());
  if (!in) return false;
 
 ofstream out(".mymake");

  string line;
  while (getline(in, line)) {
    out << "#" << line << endl;
  }

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
  out << "#Set the output path for executable files." << endl;
  out << "executablePath=./" << endl;
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
  out << "link=g++ <files> <libs> -o <output>" << endl;
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
  out << "#Debug level" << endl;
  out << "#debugLevel=1" << endl;
  out << "" << endl;
}

void Settings::parseArguments(int argc, char **argv) {
  executable = File(argv[0]).title();

  parseArguments(argc, argv, "config");
  loadSettings();
  parseArguments(argc, argv, "input");
}

void Settings::parseArguments(int argc, char **argv, const string &def) {
  if (argc <= 1) return;

  string identifier = def;

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
	debugLevel = SETTINGS;
      } else if (arg == "-d"){
	identifier = "debugLevel";
      } else if (arg == "-t") {
	showTime = true;
      } else if (arg == "-a") {
	executeCompiled = true;
	addProcessParameters(argc, argv, i + 1);
	break;
      } else {
	storeItem(identifier, arg);
	identifier = def;
      }
    }
  }

}

void Settings::addProcessParameters(int argc, char **argv, int i) {
  commandLineParams.clear();
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

  bool add = true;
  while (true) {
    size_t dot = key.find('.');
    if (dot == string::npos) break;

    string arch = key.substr(0, dot);
    key = key.substr(dot + 1);

    if (std::find(platforms.begin(), platforms.end(), arch) == platforms.end()) {
      add = false;
    }
  }

  if (add) {
    storeItem(key, value);
  }
}

void Settings::storeItem(const string &identifier, const string &value) {
  if (identifier == "input") {
    addSingle(inputFiles, value);
  } else if (identifier == "config") {
    platforms.push_back(value);
  } else if (identifier == "ext") {
    addSingle(cppExtensions, value);
  } else if (identifier == "executableExt") {
    executableExt = value;
  } else if (identifier == "out") {
    active.outFile = value;
  } else if (identifier == "executablePath") {
    executablePath = File(value);
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
  } else if (identifier == "debugLevel") {
    debugLevel = (DebugLevel)atoi(value.c_str());
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
  } else {
    PLN("Unknown identifier: " << identifier);
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
  PLN("Usage:");
  PLN(executable << " <file> [-o <output>] [-ne] [-e] [-f] [-clean] [-s] [-t] [-d level] [-a <arg1> ... <argn>]");
  PLN(executable << " -config");
  PLN(executable << " -cp");
  PLN("");
  PLN("file    : The root source file to compile (may contain multiple files).");
  PLN("output  : The name of the executable file to be created.");
  PLN("-e      : Execute the compiled file on success.");
  PLN("-ne     : Do not execute the compiled file on success.");
  PLN("-f      : Force recompilation.");
  PLN("-a      : Arguments to the started process (sets -e as well).");
  PLN("-s      : Show settings before compilation Equivalent to -d 1.");
  PLN("-t      : Show execution time at end.");
  PLN("-clean  : Remove all intermediate files.");
  PLN("-config : Setup the .mymake-file in the home directory.");
  PLN("-cp     : Copy the system wide .mymake-file to the current directory.");
  PLN("-d level: Set the debug output level to <level>.");
  PLN("");
  PLN("Debug levels (higher numbers include the lower ones):");
  PLN("0 - No output.");
  PLN("1 - Show files that are being compiled (default).");
  PLN("2 - Output settings as well as ignored files.");
  PLN("3 - Show the process of searching for includes.");
  PLN("4 - Verbose. Output a lot of internal messages.");
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
  if (active.outFile.size() == 0) return false;

  return true;
}

void Settings::outputConfig() const {
  cout << "Parameters used:" << endl;
  cout << "Input files: " << inputFiles << endl;
  cout << "Ignore: " << ignoreFiles << endl;
  cout << "Extensions: " << cppExtensions << endl;
  cout << "Output file: " << active.outFile.c_str() << endl;
  cout << "Executable extension: " << executableExt.c_str() << endl;
  cout << "Build path: " << active.buildPath.c_str() << endl;
  cout << "Compile with: " << active.compile.c_str() << endl;
  cout << "Link with: " << active.link.c_str() << endl;
  cout << "Execute file: " << (executeCompiled ? "yes" : "no") << endl;
  cout << "Force recompilation: " << (forceRecompilation ? "yes" : "no") << endl;
  cout << "Includes command: " << includeCommandLine << endl;
  cout << "Include paths: ";
  for (list<string>::const_iterator i = includePaths.begin(); i != includePaths.end(); i++) {
    cout << *i << " ";
  }
  cout << endl;
  cout << "Library command: " << libraryCommandLine << endl;
  cout << "Libraries: ";
  for (list<string>::const_iterator i = libraries.begin(); i != libraries.end(); i++) {
    cout << *i << " ";
  }
  cout << endl;

  cout << "Configuration: ";
  for (list<string>::const_iterator i = platforms.begin(); i != platforms.end(); i++) {
    cout << *i << " ";
  }
  cout << endl;

  cout << "Debug level: " << (int)debugLevel << endl;
}

bool Settings::ignoreFile(const File &file) const {
  if (ignoreFile(file.title())) {
    DEBUG(SETTINGS, "Ignored file " << file << " (" << file.title() << " matched).");
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
