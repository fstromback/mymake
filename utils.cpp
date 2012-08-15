#include "utils.h"
#include "dirent.h"
#include <stdlib.h>

using namespace std;

string quote(const string &s) {
  return "\"" + s + "\"";
}


#ifdef _WIN32

const string defaultIntermediateExt = "obj";
const bool windows = true;

string getHomeFile(const string &file) {
  char *home = getenv("HOMEPATH");
  char *drive = getenv("HOMEDRIVE");
  if ((home != 0) && (drive != 0)) {
    string h = string(drive) + string(home);
    if (h[h.size() - 1] == PATH_DELIM) {
      return h + file;
    } else {
      return h + PATH_DELIM + file;
    }
  }
  return "";
}

#else
const string defaultIntermediateExt = "obj";
const bool windows = false;

string getHomeFile(const string &file) {
  char *home = getenv("HOME");
  if (home != 0) {
    string h = home;
    if (h[h.size() - 1] == PATH_DELIM) {
      return h + file;
    } else {
      return h + PATH_DELIM + file;
    }
  }
  return "";
}



#endif
