#pragma once

#include "interval.h"

class Time {
public:
  Time();

  Interval operator-(const Time &other) const;
private:
  uint64 timestamp;
};
