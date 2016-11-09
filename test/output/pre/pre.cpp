#include <iostream>

// Outputting a lot of text when being run as a pre-build step can cause some output to go missing.
const int lines = 1000;

int main() {

	for (int i = 0; i < lines; i++)
		std::cout << "Line #" << i << std::endl;

	std::cout << "Done!" << std::endl;

	return 0;
}
