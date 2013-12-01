#pragma once

#include <string>
#include "utils.h"

using namespace std;

class Wildcard {
public:
  Wildcard(const char *);
  Wildcard(const string &str);

  bool matches(const string &str) const;

  friend ostream &operator <<(ostream &to, const Wildcard &from);
private:
  string pattern;
  bool matches(nat strAt, const string &str, nat patternAt) const;
  bool matchesStar(nat strAt, const string &str, nat patternAt) const;
};

