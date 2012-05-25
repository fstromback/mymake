#pragma once

#include <string>
#include <list>

#include "includecache.h"

using namespace std;

class Settings {
public:
  Settings();
  virtual ~Settings();

  string compile;
  string link;
  string buildPath;
  string outFile;

  string srcPath;

  string executable;

  list<string> inputFiles;
  list<string> cppExtensions;

  bool executeCompiled;
  bool forceRecompilation;
  bool showHelp;
  bool debugOutput;

  list<string> commandLineParams;

  IncludeCache cache;

  void parseArguments(int argc, char **argv);
  bool enoughForCompilation() const;

  char **getExecParams() const;

  void outputUsage() const;

  string getCompileCommand(const string &file, const string &output) const;
  string getLinkCommand(const string &files) const;
private:
  void loadFile(const string &file);
  void parseLine(const string &file);
  void storeItem(const string &identifier, const string &value);

  string replace(const string &in, const string &find, const string &replace) const;

  void addProcessParameters(int argc, char **argv, int startAt);
};
