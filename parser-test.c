#include <stdlib.h>
#include <stdio.h>
#include "parser.h"

int main(int argc, char *argv[]) {
	char *test = NULL;
	parser_config *config;
	
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [HTML string to parse]\n", argv[0]);
		return 1;
	}
	
	config = get_parser_config("parser.ini");
	test = parse_html_for_output(argv[1], config);
	free_parser_config(config);
	printf("%s\n", test);

	if (test != NULL) {
		free(test);
	}
	
	return 0;
}
