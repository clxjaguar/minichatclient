#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "cstring.h"

int main(int argc, char *argv[]) {
	const char *string;
	cstring *cs;
	char *test;
	parser_config *config;
	clist *parts;
	FILE *file;
	cs = NULL;
	int car;
	
	if (argc < 2) {
		string = ""
"<span style=\"font-weight: bold\">span bold</span> <strong>strong</strong> <i>italic</i>_\n"
"@ <span style=\"font-weight: bold\">FeNr1s</span>, LU' from fenris1\n"
"@ <span><span>FeNr1s_modo</span></span>, LU' from fenris modo\n"
"@ <span style=\"font-weight: bold\">FeNr1s</span>, <span style=\"font-weight: bold\">FeNr1s</span>, LU' from fenris 3\n"
"<strong>Nouveau Sujet</strong> -- <a href=\"http://forum.francefurs.org/viewtopic.php?p_176959#p176959\" class=\"postlink\">Un jeu plein de furries pour l'été :3</a>\n"
"<!-- m --><a class=\"postlink\" href=\"http://www.youtube.com/watch?v_0YJR-doOWf8\">http://www.youtube.com/watch?v_0YJR-doOWf8</a><!-- m -->\n"
"<img src=\"http://t3.gstatic.com/images?q_tbn:ANd9GcRFSm3Ll9aQLTiG6xiJ1uRXr90ws-NtHWiWZO44sSOjihPfWCrM&t=1\" alt=\"Image\" />\n"
"";
	} else {
		if (argc < 3) {
			string = argv[1];
		} else if (!strcmp(argv[1], "-f")) {
			file = fopen(argv[2], "r");
			if (!file) {
				fprintf(stderr, "Cannot open the given file.");
				return 2;
			}
			cs = new_cstring();
			while (!feof(file)) {
				car = fgetc(file);
				if (car >= 0) {
					cstring_addc(cs, (char)car);
				}
			}
			string = cs->string;
		} else {
			fprintf(stderr, "Syntax not correct.\nUsage:\n\t%s "
				"[string to parse]\n\t%s -f [file to parse]\n"
				, argv[0], argv[0]);
			return 1;
		}
	}
	
	config = get_parser_config("parser_rules.conf");
	parts = get_parser_parts(string);
	test = parse_html_in_message(parts, config);
	free_clist(parts);
	free_parser_config(config);
	printf("%s\n", test);

	if (test != NULL) {
		free(test);
	}
	
	if (cs != NULL) {
		free_cstring(cs);
	}
	
	return 0;
}
