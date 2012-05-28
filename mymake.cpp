#include <sys/types.h>
#include <dirent.h>

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include "directory.h"
#include "files.h"
#include "cppfile.h"
#include "globals.h"
#include "utils.h"

using namespace std;

bool runCommand(const string &commandline) {
  if (settings.showSettings) cout << commandline.c_str() << endl;
  int returnCode = system(commandline.c_str());
  if (returnCode != 0) {
    cout << "Failed: " << returnCode << endl;
  }
  return returnCode == 0;
}

bool compileFiles(Files &files, Files &toLink) {
  for (list<File>::iterator i = files.begin(); i != files.end(); i++) {
    CppFile file(*i);

    for (list<string>::iterator type = settings.cppExtensions.begin(); type != settings.cppExtensions.end(); type++) {
      files.append(file.getIncludes().changeFiletypes(*type));
    }

    if (file.isValid()) {
      File output = file.modifyRelative(settings.srcPath, settings.getBuildPath()).modifyType("o");
       
      bool needsCompilation = false;
      if (settings.forceRecompilation) {
	needsCompilation = true;
      } else if (!output.isValid()) {
	needsCompilation = true;
      } else if (file.getLastModified() > output.getLastModified()) {
	needsCompilation = true;
      }

      if (needsCompilation) {
	cout << "Compiling " << *i << "..." << endl;

	output.ensurePathExists();

	string compileArgument = settings.getCompileCommand(quote(file.getFullPath()), quote(output.getFullPath()));
	if (!runCommand(compileArgument)) {
	  return false;
	}
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
  if (!executable.isValid()) {
    link = true;
  } else if (executable.getLastModified() < files.getLastModified()) {
    link = true;
  } else {
    cout << "Executable up to date!\n";
  }

  if (link) {
    cout << "Linking " << settings.getOutFile() << "..." << endl;

    stringstream toLink;
    bool first = true;
    for (list<File>::iterator i = files.begin(); i != files.end(); i++) {
      if (!first) toLink << " ";
      toLink << quote(i->getFullPath());
      first = false;
    }


    if (!runCommand(settings.getLinkCommand(toLink.str()))) return false;
  }

  return true;
}

void addFile(const string &file, Files &to) {
  File f(file);
  if (f.isValid()) {
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
  return buildDirectory.remove();
}

int main(int argc, char **argv) {

  settings.parseArguments(argc, argv);

  if (!settings.enoughForCompilation() || settings.showHelp) {
    settings.outputUsage();
    return -1;
  }

  if (settings.doInstall) {
    settings.install();
    return 0;
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
    if (!settings.cache.load()) { //Ladda cachen
      cout << "Failed to load the cache!" << endl;
    }
  }

  Files files;
  for (list<string>::iterator i = settings.inputFiles.begin(); i != settings.inputFiles.end(); i++) {
    addFileExts(*i, files);
  }

  if (files.size() == 0) {
    cout << "Error: File not found." << endl;
    return -1;
  }

  list<File>::iterator i = files.begin();
  settings.srcPath = i->getDirectory();

  int errorCode = 0;

  Files toLink;
  if (compileFiles(files, toLink)) {
    if (linkOutput(toLink)) {
      cout << "Build successful!\n";
    } else {
      cout << "Build failed!\n";
      errorCode = -2;
    }
  } else {
    cout << "Build failed!\n";
    errorCode = -1;
  }

  settings.cache.save();

  if (errorCode == 0) {
    if (settings.executeCompiled) {
      execv(settings.getOutFile().c_str(), settings.getExecParams());
    }
  }
  return errorCode;
}

