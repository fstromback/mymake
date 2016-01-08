#include <cstdio>
#include "io/print.h"

int main() {
	printf("Numbers:\n");

	for (int i = 0; i < 5; i++) {
		print(i);
	}

	printf("Characters:\n");

	for (char i = 'a'; i < 'g'; i++) {
		print(i);
	}

	printf("Done!\n");

	return 0;
}
