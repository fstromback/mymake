#include "wildcard.h"

Wildcard::Wildcard(const string &pattern) : pattern(pattern) {}

Wildcard::Wildcard(const char *pattern) : pattern(pattern) {}

bool Wildcard::matches(const string &str) const {
  return matches(0, str, 0);
}

bool Wildcard::matches(int strAt, const string &str, int patternAt) const {
  for (; patternAt < pattern.length(); patternAt++) {
    if (strAt == str.length()) return false; //premature ending
    
    switch (pattern[patternAt]) {
    case '*':
      return matchesStar(strAt, str, patternAt + 1);
      break;
    case '?':
      strAt++;
      break;
    default:
      if (pattern[patternAt] != str[strAt]) return false;
      strAt++;
      break;
    }
  }

  return strAt == str.length();
}

bool Wildcard::matchesStar(int strAt, const string &str, int patternAt) const {
  for (; strAt <= str.length(); strAt++) {
    if (matches(strAt, str, patternAt)) return true;
  }
  return false;
}

ostream &operator <<(ostream &to, const Wildcard &w) {
  return to << w.pattern;
}