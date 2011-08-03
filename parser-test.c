#include <stdlib.h>
#include <stdio.h>
#include "parser.h"

int main(int argc, char *argv[]) {
	char *string;
	char *test;
	parser_config *config;
	
	if (argc < 2) {
		string = ""
"<span style=\"font-weight: bold\">span bold</span> <strong>strong</strong> <i>italic</i>\n"
"@ <span style=\"font-weight: bold\">FeNr1s</span>, LU'\n"
"<strong>Nouveau Sujet</strong> -- <a href=\"http://forum.francefurs.org/viewtopic.php?p_176959#p176959\" class=\"postlink\">Un jeu plein de furries pour l'été :3</a>\n"
"<!-- m --><a class=\"postlink\" href=\"http://www.youtube.com/watch?v_0YJR-doOWf8\">http://www.youtube.com/watch?v_0YJR-doOWf8</a><!-- m -->\n"
"<img src=\"http://t3.gstatic.com/images?q_tbn:ANd9GcRFSm3Ll9aQLTiG6xiJ1uRXr90ws-NtHWiWZO44sSOjihPfWCrM&t=1\" alt=\"Image\" />\n"
"";
	} else {
		string = argv[1];
	}
	
	config = get_parser_config("parser_rules.conf");
	test = parse_html_in_message(string, config);
	free_parser_config(config);
	printf("%s\n", test);

	if (test != NULL) {
		free(test);
	}
	
	return 0;
}
