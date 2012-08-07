#pragma once

#include <iostream>

using namespace std;

typedef unsigned long long uint64;

class Interval {
public:
  Interval(uint64 time);

  inline Interval operator +(const Interval &other) const { return Interval(time + other.time); };
  inline Interval operator -(const Interval &other) const { return Interval(time - other.time); };

  friend ostream &operator <<(ostream &to, const Interval &from);
private:
  //in us
  uint64 time;
};
