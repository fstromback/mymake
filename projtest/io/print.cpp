#include <cstdio>
#include "print.h"

void print(char nr) {
	printf("char: %c (%d)\n", nr, int(nr));
}

void print(int nr) {
	printf("int: %d\n", nr);
}

void print(float nr) {
	printf("float: %f\n", nr);
}

