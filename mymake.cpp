#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include "directory.h"
#include "files.h"
#include "cppfile.h"
#include "globals.h"
#include "utils.h"
#include "time.h"

#include "unistd.h"

//for testing of cache and ignore features.
#include "dummy.h"

using namespace std;

//total compilation time.
Interval compilationTime(0);

bool runCommand(const string &commandline) {
  DEBUG(COMPILATION, "Running: " << commandline);

  Time start;
  int returnCode = system(commandline.c_str());
  compilationTime = compilationTime + (Time() - start);

  if (returnCode != 0) {
    cout << "Failed: " << returnCode << endl;
  }
  return returnCode == 0;
}

bool compileFiles(Files &files, Files &toLink) {
  File srcPath = File();
  for (list<File>::iterator i = files.begin(); i != files.end(); i++) {
    CppFile file(*i);

    for (list<string>::iterator type = settings.cppExtensions.begin(); type != settings.cppExtensions.end(); type++) {
      files.append(file.getIncludes().changeFiletypes(*type));
    }

    DEBUG(VERBOSE, "Files: " << endl << files);

    if (file.exists() && !settings.ignoreFile(file)) {
      File relative = file.makeRelative(srcPath);
      File output = File(settings.getBuildPath()) + relative;
      output.setType(settings.intermediateExt);
       
      bool needsCompilation = false;
      if (settings.forceRecompilation) {
	needsCompilation = true;
      } else if (!output.exists()) {
	needsCompilation = true;
      } else if (file.getLastModified() > output.getLastModified()) {
	needsCompilation = true;
      }

      if (needsCompilation) {
	DEBUG(DEFAULT, "Compiling " << *i << "...");

	output.ensurePathExists();

	string compileArgument = settings.getCompileCommand(quote(file.toString()), quote(output.toString()), file.getType());
	if (!runCommand(compileArgument)) {
	  return false;
	}
      } else {
	DEBUG(VERBOSE, file << " did not need to be compiled.");
      }

      toLink.add(File(output));
    }
  }

  return true;
}

bool linkOutput(Files &toLink) {
  //Files files(Directory(outputPath), ".o");
  Files &files = toLink;

  File executable(settings.getOutFile());
  executable.ensurePathExists();

  bool link = false;
  if (!executable.exists()) {
    link = true;
  } else if (executable.getLastModified() < files.getLastModified()) {
    link = true;
  } else {
    DEBUG(DEFAULT, "Executable up to date!");
  }

  if (link) {
    DEBUG(DEFAULT, "Linking " << settings.getOutFile() << "...");

    stringstream toLink;
    bool first = true;
    for (list<File>::iterator i = files.begin(); i != files.end(); i++) {
      if (!first) toLink << " ";
      toLink << quote(i->toString());
      first = false;
    }


    if (!runCommand(settings.getLinkCommand(toLink.str()))) return false;
  }

  return true;
}

void addFile(const string &file, Files &to) {
  File f(file);

  if (f.isAbsolute()) {
    DEBUG(VERBOSE, "Translating " << f);
    f = f.makeRelative(File::cwd());
    DEBUG(VERBOSE, "Into " << f);
  }

  if (f.exists()) {
    for (list<string>::iterator i = settings.cppExtensions.begin(); i != settings.cppExtensions.end(); i++) {
      if (f.isType(*i)) {
	to.add(f);
	break;
      }
    }
  }
}

void addFileExts(const string &file, Files &to) {
  addFile(file, to);
  for (list<string>::iterator i = settings.cppExtensions.begin(); i != settings.cppExtensions.end(); i++) {
    addFile(file + "." + *i, to);
  }
}

bool clean() {
  //Remove all files in the build directory.
  File buildDirectory = File(settings.getBuildPath());
  return buildDirectory.deleteFile();
}

int main(int argc, char **argv) {
  Time startTime;

  settings.parseArguments(argc, argv);

  if (settings.doInstall) {
    settings.install();
    return 0;
  } else if (settings.doCopySettings) {
    if (!settings.copySettings()) {
      cout << "Failed to copy settings." << endl;
    }
    return 0;
  }

  // Generate the input files so that we can supply an output file if it is missing.
  Files files;
  for (list<string>::iterator i = settings.inputFiles.begin(); i != settings.inputFiles.end(); i++) {
    addFileExts(*i, files);
  }

  if (settings.getOutFile() == "" && files.size() > 0) {
    // Set the output file to the first found input file with modified extention.
    File inFile = *files.begin();
    File outFile(settings.executablePath);
    outFile += inFile.title();
    outFile.setType(settings.executableExt);
    settings.setOutFile(outFile.toString());
  }

  if (!settings.enoughForCompilation() || settings.showHelp) {
    settings.outputUsage();
    return -1;
  }

  if (settings.debugLevel >= SETTINGS) {
    settings.outputConfig();
  }

  if (settings.clean) {
    if (clean()) {
      cout << "Cleaned intermediate files." << endl;
      return 0;
    } else {
      cout << "Cleaning failed!" << endl;
      return -1;
    }
  }

  if (!settings.forceRecompilation) {
    if (!settings.cache.load()) { // Load the cache
      cout << "Failed to load the cache!" << endl;
    }
  }

  if (files.size() == 0) {
    cout << "Error: No files to compile." << endl;
    return -1;
  }

  int errorCode = 0;

  Files toLink;
  if (compileFiles(files, toLink)) {
    if (linkOutput(toLink)) {
      DEBUG(DEFAULT, "Build successful!");
    } else {
      PLN("Build failed!");
      errorCode = -2;
    }
  } else {
    PLN("Build failed!");
    errorCode = -1;
  }

  settings.cache.save();

  Interval totalTime = Time() - startTime;

  if (settings.showTime) {
    cout << "Total time : " << totalTime << endl;
    cout << "   compiler: " << compilationTime << endl;
    cout << "   mymake  : " << (totalTime - compilationTime) << endl;
  }

  cout.flush();

  if (errorCode == 0) {
    if (settings.executeCompiled) {
      execv(settings.getOutFile().c_str(), settings.getExecParams());
    }
  }

  return errorCode;
}

