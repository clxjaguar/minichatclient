#include <stdlib.h>
#include <stdio.h>
#include "parser.h"

int main(int argc, char *argv[]) {
	char *test = NULL;
	
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [HTML string to parse]\n", argv[0]);
		return 1;
	}
	
	test = parse_html_for_output(argv[1]);
	printf("%s\n", test);

	free(test); test = NULL;
	return 0;
}
