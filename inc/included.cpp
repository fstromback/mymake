#include "included.h"
#include "settings.h"
#include "globals.h"

#include <iostream>

void includePathTest() {
  if (settings.debugOutput) std::cout << "Includes are working!" << std::endl;
}
