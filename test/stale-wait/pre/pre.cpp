#include <iostream>
#include <windows.h>

// Sometimes when the compiler outputs much data, mm will keep waiting for the process to terminate
// even though that already happened.
const int lines = 200;

int main() {

	for (int i = 0; i < lines; i++)
		std::cout << "Line #" << i << std::endl;

	std::cout << "Done!" << std::endl;

	return 0;
}
