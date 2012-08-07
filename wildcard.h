#pragma once

#include <string>

using namespace std;

class Wildcard {
public:
  Wildcard(const char *);
  Wildcard(const string &str);

  bool matches(const string &str) const;

  friend ostream &operator <<(ostream &to, const Wildcard &from);
private:
  string pattern;
  bool matches(int strAt, const string &str, int patternAt) const;
  bool matchesStar(int strAt, const string &str, int patternAt) const;
};

