#include <iostream>
#include <sstream>

#if defined(_WIN32)
#include <Windows.h>

void work() {
	Sleep(500);
}

#else
#include <unistd.h>

void work() {
	usleep(500 * 1000);
}

#endif


// Outputting a lot of text when being run as a pre-build step, or elsewhere can cause some output to go missing.
const int lines = 100;


int main(int argc, const char *argv[]) {
	(void)argv;

	// Simulate working hard...
	work();

	// Make a large buffer and output everything at once!
	std::ostringstream out;
	for (int i = 0; i < lines; i++)
		out << "Line #" << i << std::endl;

	out << "Done!" << std::endl;

	// Turn of buffering:
	setvbuf(stdout, 0, _IONBF, 10*4096);
	std::cout << out.str();
	std::cout.flush();

	// Simulate failure if asked to do so.
	if (argc > 1)
		return 1;

	return 0;
}
