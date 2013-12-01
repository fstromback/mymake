#pragma once

#include <string>
#include <list>

#include "includecache.h"
#include "wildcard.h"
#include "file.h"
#include "debug.h"

using namespace std;

class Settings {
public:
  Settings();
  virtual ~Settings();

  void loadSettings();

  string intermediateExt;
  string executableExt;
  string executable;
  File executablePath;

  list<string> inputFiles;
  list<string> cppExtensions;

  bool executeCompiled;
  bool forceRecompilation;
  bool showHelp;
  DebugLevel debugLevel;
  bool showTime;

  bool clean;
  bool doInstall;
  bool doCopySettings;

  list<string> includePaths;
  string includeCommandLine;

  list<string> libraries;
  string libraryCommandLine;

  list<string> commandLineParams;

  list<string> platforms;

  IncludeCache cache;

  bool ignoreFile(const File &f) const;

  void parseArguments(int argc, char **argv);
  bool enoughForCompilation();

  char **getExecParams() const;

  void outputUsage() const;

  void install() const; //Setup the .mymake file in the user's home-directory.
  bool copySettings() const; //Copt the global .mymake to the current directory.

  string getIncludeString() const;
  string getLibString() const;
  string getCompileCommand(const string &file, const string &output) const;
  string getLinkCommand(const string &files) const;

  inline string getBuildPath() const { return active.buildPath; }
  inline string getOutFile() const { return active.outFile; }
  inline void setOutFile(const string &f) { active.outFile = f; }

  void outputConfig() const;
private:
  list<Wildcard> ignoreFiles;

  class Configuration {
  public:
    string compile;
    string link;
    string buildPath;
    string outFile;
  };

  Configuration active;

  bool ignoreFile(const string &title) const;
    
  void loadFile(const string &file);
  void parseLine(const string &file);
  void storeItem(const string &identifier, const string &value);

  string replace(const string &in, const string &find, const string &replace) const;

  void addProcessParameters(int argc, char **argv, int startAt);

  void parseArguments(int argc, char **argv, const string &def);

  static list<string> parseCommaList(const string &str);
};
