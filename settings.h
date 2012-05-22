#pragma once

#include <string>
#include <list>

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

  void parseArguments(int argc, char **argv);
  bool enoughForCompilation() const;

  void outputUsage() const;

  string getCompileCommand(const string &file, const string &output) const;
  string getLinkCommand(const string &files) const;
private:
  void loadFile(const string &file);
  void parseLine(const string &file);
  void storeItem(const string &identifier, const string &value);
};
