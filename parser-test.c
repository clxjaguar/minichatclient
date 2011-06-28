#include <stdlib.h>
#include <stdio.h>
#include "parser.h"

int main(int argc, char *argv[]) {
	char *string;
	char *test;
	parser_config *config;
	
	if (argc < 2) {
		string = "<span style=\"font-weight: bold\">span bold</span> <strong>strong</strong> <i>'i'</i>";
	} else {
		string = argv[1];
	}
	
	config = get_parser_config("parser_plaintext.conf");
	test = parse_html_in_message(string, config);
	free_parser_config(config);
	printf("%s\n", test);

	if (test != NULL) {
		free(test);
	}
	
	return 0;
}
