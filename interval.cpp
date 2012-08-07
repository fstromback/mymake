#include "interval.h"
#include <iomanip>

Interval::Interval(uint64 t) : time(t) {}

ostream &operator <<(ostream &to, const Interval &from) {
  uint64 t = from.time;
  if (t < 1000) {
    cout << t << " us";
  } else if (t < 1000LL * 1000LL) {
    cout << fixed << setprecision(2) << (t / 1000.0) << " ms";
  } else {
    cout << fixed << setprecision(2) << (t / 1000.0 / 1000.0) << " s";
  }
  return to;
}
