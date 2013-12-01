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

  if (!file.exists()) {
    cout << "Error: The file " << file.toString().c_str() << " does not exist." << endl;
    return result;
  }

  ifstream* f = file.read();
  if (f == 0) {
    cout << "Error: Failed to open the file " << file.toString() << "." << endl;
    return result;
  }

  DEBUG(INCLUDES, "Processing " << file << "...");

  while (!f->eof()) {
    string line;
    getline(*f, line);

    vector<string> tokens = parseLine(line);
    if (tokens.size() > 1) {
      if (tokens[0] == "#include") {
	string &token = tokens[1];
	if (token[0] == '"') {
	  if (token[token.size() - 1] == '"') {
	    string name = token.substr(1, token.size() - 2);

	    DEBUG(INCLUDES, "Found: " << name);

	    if (result.addCppHeader(file.parent() + name)) {
	    } else if (result.addCppHeader(name)) {
	    } else {
	      PLN("In " << file << ": The included file " << name << " does not exist.");
	    }
	  }
	}
      }
    }
  }
      
  delete f;

  return result;
}

bool Files::addCppHeader(const File &toAdd) {
  if (toAdd.exists()) {
    add(toAdd);
    DEBUG(VERBOSE, "Included file " << toAdd << " found!");
    return true;
  } else {
    DEBUG(VERBOSE, "Included file " << toAdd << " not found!");
    return false;
  }
}

bool Files::addCppHeader(const string &name) {
  DEBUG(VERBOSE, "Looking for " << name << "...");
  for (list<string>::iterator i = settings.includePaths.begin(); i != settings.includePaths.end(); i++) {
    if (addCppHeader(File(*i) + name)) return true;
  }
  return false;
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
    if (i->verifyDir()) {
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

void Files::output(ostream &to) const {
  for (list<File>::const_iterator i = files.begin(); i != files.end(); i++) {
    const File *f = &(*i);
    to << *f;
    to << endl;
  }
}

ostream &operator <<(ostream &to, const Files &o) {
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
    File modified = *i;
    modified.setType(to);
    if (modified.exists()) ret.files.push_back(modified);
  }

  return ret;
}


bool Files::load(const File &file) {
  string f = file.toString();

  if (settings.cache.files.count(f) <= 0) return false;

  IncludeCache::CachedCpp &cpp = settings.cache.files[f];

  if (cpp.modified < file.getLastModified()) {
    DEBUG(VERBOSE, "The file " << file <<
	  " have been modified from " << cpp.modified <<
	  " to " << file.getLastModified() << ". Discarding old file cache.");
    return false;
  }
  
  bool valid = true;
  for (list<IncludeCache::CachedFile>::const_iterator i = cpp.includedFiles.begin(); i != cpp.includedFiles.end(); i++) {
    File incFile = File(i->file);
    if (i->modified < incFile.getLastModified()) {
      DEBUG(VERBOSE, "The file " << incFile << " have been modified from " <<
	    i->modified << " to " << incFile.getLastModified() << ". Renewing cache.");
      valid = false;
      break;
    }

    files.push_back(incFile);
  }

  if (!valid) files.clear();

  return valid;
}

void Files::save(const File &file) const {
  IncludeCache::CachedCpp &cpp = settings.cache.files[file.toString()];

  cpp.file = file.toString();
  cpp.modified = file.getLastModified();

  cpp.includedFiles.clear();

  for (list<File>::const_iterator i = files.begin(); i != files.end(); i++) {
    File f(*i);
    DEBUG(VERBOSE, "Stored file: " << f << ", time " << f.getLastModified());
    cpp.includedFiles.push_back(IncludeCache::CachedFile(f.toString(), f.getLastModified()));
  }
}
