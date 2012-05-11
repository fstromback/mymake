#include <sys/types.h>
#include <dirent.h>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "directory.h"
#include "files.h"
#include "cppfile.h"
#include "globals.h"

void listDir();

using namespace std;

bool parseArguments(int argc, char **argv) {
  if (argc <= 1) return false;

  for (int i = 1; i < argc - 1; i += 2) {
    string arg = string(argv[i]);
    
    if (arg == "-s") {
      srcPath = argv[i + 1];
    } else if (arg == "-o") {
      outputPath = argv[i + 1];
    } else if (arg == "-e") {
      executablePath = argv[i + 1];
    } else if (arg == "-c") {
      compiler = argv[i + 1];
    } else {
      cout << "Unknown command-line argument: " << arg << endl;
      return false;
    }
  }
  return true;
}

void printUsage() {
  cout << "Usage:" << endl;
  cout << "mymake -s <source path> -o <output path> -e <executable file> [-c <compiler>]" << endl << endl;
  cout << "source path     : The path where the source files are located." << endl;
  cout << "output path     : The path where the intermediate output files will be located." << endl;
  cout << "executable file : The filename of the final executable file." << endl;
  cout << "compiler        : The name of the compiler to run. The default is g++." << endl;
}

bool runCommand(const string &commandline) {
  int returnCode = system(commandline.c_str());
  if (returnCode != 0) {
    cout << "Failed: " << returnCode << endl;
  }
  return returnCode == 0;
}

bool compileFiles(Files &files, Files &toLink) {
  for (list<File>::iterator i = files.begin(); i != files.end(); i++) {
    CppFile file(*i);
    if (addHFiles) {
      files.append(file.getIncludes().changeFiletypes("cpp"));
    }

    if (file.isValid()) {
      File output = file.modifyRelative(srcPath, outputPath).modifyType("o");
       
      bool needsCompilation = false;
      if (!output.isValid()) {
	needsCompilation = true;
      } else if (file.getLastModified() > output.getLastModified()) {
	needsCompilation = true;
      }

      if (needsCompilation) {
	cout << "Compiling " << *i << "..." << endl;

	output.ensurePathExists();

	string compileArgument = compiler + " " + file.getFullPath() + " -c -o " + output.getFullPath();
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

  File executable(executablePath);
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
    cout << "Linking " << executablePath << "..." << endl;

    string linkArgument = compiler;
    for (list<File>::iterator i = files.begin(); i != files.end(); i++) {
      linkArgument = linkArgument + " " + i->getFullPath();
    }


    if (!runCommand(linkArgument + " -o " + executablePath)) return false;
  }

  return true;
}

int main(int argc, char **argv) {

  if (!parseArguments(argc, argv)) {
    printUsage();
    return -1;
  }

  Files files(Directory(srcPath), ".cpp");


  File srcPathFile(srcPath);
  if (!srcPathFile.isDirectory()) {
    srcPath = srcPathFile.getDirectory();
    addHFiles = true;
  }
  
  Files toLink;
  if (compileFiles(files, toLink)) {
    if (linkOutput(toLink)) {
      cout << "Build successful!\n";
    } else {
      cout << "Build failed!\n";
      return -2;
    }
  } else {
    cout << "Build failed!\n";
    return -1;
  }
 
  return 0;
}

