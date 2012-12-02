#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "CUtils/cstring.h"

#define bool int
#define true 1
#define false 0

int main(int argc, char *argv[]) {
	const char *string;
	cstring *cs;
	char *test;
	parser_config *config;
	FILE *file;
	cs = NULL;
	int car;
	bool cont;
	char *new_argv[2];
	
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
		} else if (!strcmp(argv[1], "-F")) {
			file = fopen(argv[2], "r");
			if (!file) {
				fprintf(stderr, "Cannot open the given file.");
				return 2;
			}
			
			printf("Reading file...\n");
			cs = new_cstring();
			while (!feof(file)) {
				car = fgetc(file);
				if (car >= 0 && car != '\r') {
					cstring_addc(cs, (char)car);
				}
			}
			string = cs->string;
			printf("Done!\n");
		} else if (!strcmp(argv[1], "-f")) {
			file = fopen(argv[2], "r");
			if (!file) {
				fprintf(stderr, "Cannot open the given file.");
				return 2;
			}
			cs = new_cstring();
			cont = true;
			while (cont && !feof(file)) {
				car = fgetc(file);
				if (car >= 0 && car != '\r') {
					if (car == '\n') {
						new_argv[0] = argv[0];
						new_argv[1] = cs->string;
						cont = !main(2, new_argv);
						cstring_compact(cs);
						cstring_clear(cs);
					} else {
						cstring_addc(cs, (char)car);
					}
				}
			}
			if (car >= 0 && car != '\n') {
				new_argv[0] = argv[0];
				new_argv[1] = cs->string; 
				cont = !main(2, new_argv);
			}
			free_cstring(cs);
			cs = NULL;
			string = NULL;
		} else {
			fprintf(stderr, "Syntax not correct.\nUsage:\n"
				"\t%s [string to parse]\n"
				"\t%s -f [file to parse (line per line)]\n"
				"\t%s -F [file to parse (at once)]\n"
				, argv[0], argv[0], argv[0]);
			return 1;
		}
	}
	
	if (string != NULL) {
		test = NULL;
		config = parser_get_config("parser_rules.conf");
		test = parser_parse_html(string, config);
		free_parser_config(config);
		printf("%s\n", test);
		
		if (test != NULL) {
			free(test);
		}
	}
	
	if (cs != NULL) {
		free_cstring(cs);
	}
	
	return 0;
}
