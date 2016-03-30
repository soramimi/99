#include <stdio.h>

int main(int argc, char *argv[])
{
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			int n = (i + 1) * (j + 1);
			printf(" %2d", n);
		}
		putchar('\n');
	}
	return 0;
}
