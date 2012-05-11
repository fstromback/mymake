#include "files.h"

using namespace std;

Files::Files() {}

Files::Files(const Directory &folder, string type) {
  addFiles(folder, type);
}

Files::Files(const File &file) {
  ifstream* f = file.read();

  while (!f->eof()) {
    string line;
    getline(*f, line);

    vector<string> tokens = parseLine(line);
    if (tokens[0] == "#include") {
      string &token = tokens[1];
      if (token[0] == '"') {
	if (token[token.size() - 1] == '"') {
	  files.push_back(File(file.getDirectory(), token.substr(1, token.size() - 2)));
	}
      }
    }
  }
      
  delete f;
}

Files::~Files() {}


vector<string> Files::parseLine(const string &line) {
  list<string> tokens;
  bool inString = false;
  string current = "";
  for (int i = 0; i < line.size(); i++) {
    if (inString) {
      current = current + string(1, line[i]);
      if (line[i] == '"') inString = false;
    } else if (line[i] == ' ') {
      if (current != "") tokens.push_back(current);
      current = "";
    } else {
      current = current + string(1, line[i]);
    }
  }

  if (current != "") tokens.push_back(current);

  return vector<string>(tokens.begin(), tokens.end());
}


void Files::addFiles(const Directory &folder, string type) {
  //cout << "Adding folders to \"" << folder.getPath() << "\"...\n";
  for (list<File>::const_iterator iter = folder.folders.begin(); iter != folder.folders.end(); iter++) {
    const File *i = &(*iter);
    if (i->isDirectory()) {
      if (!i->isPrevious()) {
	addFiles(Directory(*i), type);
      }
    }
  }

  //cout << "Adding files to \"" << folder.getPath() << "\"...\n";
  for (list<File>::const_iterator iter = folder.files.begin(); iter != folder.files.end(); iter++) {
    const File *i = &(*iter);
    if (i->isType(type)) files.push_back(*i);
  }

  //cout << "Done adding files to \"" << folder.getPath() << "\"\n";
}

void Files::output(ostream &to) {
  for (list<File>::iterator i = files.begin(); i != files.end(); i++) {
    File *f = &(*i);
    to << *f;
    to << endl;
  }
}

ostream &operator <<(ostream &to, Files &o) {
  o.output(to);
  return to;
}

void Files::append(const Files &files) {
  
  for (list<File>::const_iterator i = files.files.begin(); i != files.files.end(); i++) {
    bool found = false;
    for (list<File>::iterator j = this->files.begin(); j != this->files.end(); j++) {
      if (*i == *j) {
	found = true;
	break;
      }
    }

    if (!found) this->files.push_back(*i);
  }
}

void Files::add(const File &file) {
  files.push_back(file);
}

time_t Files::getLastModified() const {
  time_t latestTime = 0;
  for (list<File>::const_iterator i = files.begin(); i != files.end(); i++) {
    if (latestTime < i->getLastModified()) {
      latestTime = i->getLastModified();
    }
  }

  return latestTime;
}

Files Files::changeFiletypes(const string &to) {
  Files ret;
  
  for (list<File>::const_iterator i = files.begin(); i != files.end(); i++) {
    File modified = i->modifyType(to);
    if (modified.isValid()) ret.files.push_back(i->modifyType(to));
  }

  return ret;
}
