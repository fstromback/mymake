#include <iostream>
#include <string>

#include "wildcard.h"

using namespace std;

//Test for the  "Wildcard" class.
void test(const Wildcard &pattern, const string &match) {
  cout << pattern;
  cout << (pattern.matches(match) ? " matches " : " does not match ");
  cout << match << endl;
}

int main() {
  cout << "Testing the question mark:" << endl;
  
  test("abc", "abc");
  test("ab?", "abc");
  test("a?c", "abc");
  test("???", "abc");
  test("????", "abc");
  test("???", "abcd");

  cout << "Testing the star:" << endl;
  test("a*bc", "abc");
  test("a*d", "abcd");
  test("*d", "abcd");
  test("a*", "abcd");
  test("ac*", "abc");
  test("ab*a", "abcd");
  test("*.template.*", "stack.template.cc");

  return 0;
}
