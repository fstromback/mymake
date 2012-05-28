#include <iostream>
#include <string>
#include <list>

using namespace std;

template <class T>
void outputItem(const T &item, bool first, ostream &to) {
  if (!first) to << " ";
  to << item;
}

template <>
void outputItem(const string &item, bool first, ostream &to) {
  if (!first) to << " ";

  to << item.c_str();
}

template <class T>
void outputList(const T &start, const T &end, ostream &to = cout) {
  bool first = true;
  for (T i = start; i != end; i++) {
    outputItem(*i, first, to);
    first = false;
  }
}

template <class T>
ostream &operator <<(ostream &to, const list<T> &output) {
  outputList(output.begin(), output.end(), to);
  return to;
}

