#include "files.h"

#include "globals.h"

using namespace std;

Files::Files() {}

Files::Files(const Directory &folder, string type) {
  addFiles(folder, type);
}

Files::~Files() {}

Files Files::loadFromCpp(const File &file) {
  Files result;

  ifstream* f = file.read();

  while (!f->eof()) {
    string line;
    getline(*f, line);

    vector<string> tokens = parseLine(line);
    if (tokens[0] == "#include") {
      string &token = tokens[1];
      if (token[0] == '"') {
	if (token[token.size() - 1] == '"') {
	  result.add(File(file.getDirectory(), token.substr(1, token.size() - 2)));
	  //files.push_back(File(file.getDirectory(), token.substr(1, token.size() - 2)));
	}
      }
    }
  }
      
  delete f;

  return result;
}


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
    if (i->isType(type)) add(*i); //files.push_back(*i);
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
   bool found = false;
   for (list<File>::iterator j = this->files.begin(); j != this->files.end(); j++) {
     if (file == *j) {
       found = true;
       break;
     }
   }

   if (!found) this->files.push_back(file);
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


bool Files::load(const File &file) {
  string f = file.getFullPath();

  if (settings.cache.files.count(f) <= 0) return false;

  IncludeCache::CachedCpp &cpp = settings.cache.files[f];

  if (cpp.modified < file.getLastModified()) {
    cout << "The file " << file.getTitle().c_str() << 
          " have been modified from " << cpp.modified << " to " << 
          file.getLastModified() << ". Renewing cache..." << endl;
    return false;
  }
  
  bool valid = true;
  for (list<IncludeCache::CachedFile>::const_iterator i = cpp.includedFiles.begin(); i != cpp.includedFiles.end(); i++) {
    File incFile = File(i->file);
    if (i->modified < incFile.getLastModified()) {
      if (settings.debugOutput) {
	cout << "The file " << incFile.getTitle().c_str() << 
          " have been modified from " << i->modified << " to " << 
          incFile.getLastModified() << ". Renewing cache..." << endl;
      }
      valid = false;
      break;
    }

    files.push_back(incFile);
  }

  if (!valid) files.clear();

  return valid;
}

void Files::save(const File &file) const {
  IncludeCache::CachedCpp &cpp = settings.cache.files[file.getFullPath()];

  cpp.file = file.getFullPath();
  cpp.modified = file.getLastModified();

  cpp.includedFiles.clear();

  for (list<File>::const_iterator i = files.begin(); i != files.end(); i++) {
    File f(*i);
    if (settings.debugOutput) cout << "Stored file: " << f.getTitle().c_str() << ", time " << f.getLastModified() << endl;
    cpp.includedFiles.push_back(IncludeCache::CachedFile(f.getFullPath(), f.getLastModified()));
  }
}
