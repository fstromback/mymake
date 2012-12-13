#include <iostream>

#include "time.h"

void test(int max) {
  Time t;
  for (int i = 0; i < max; i++);
  Interval u = Time() - t;
  cout << u << endl;
}

int main() {
  test(100000);
  test(10000);
  test(1000);
  test(100);
  test(10);
  test(1);

  return 0;
}
