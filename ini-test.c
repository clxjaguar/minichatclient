/*
  Name:        ini.c
  Copyright:   niki (cc-by-nc) 2011
  Author:      niki
  Date:        13/06/11 10:46
  Description: Unit test for ini.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ini.h"
#include "ini_p.h"

/**
 * Unit test for ini.
 */
int main (int argc, char *argv[]) {
	attribute_t ** atts;
	FILE *file;
	attribute_t *att;
	int i;
	char *name, *value;
	
	if (argc != 2) {
		fprintf(stderr, "Usage: %s FILE.INI\n", argv[0]);
		return 1;
	}
	
	file = fopen(argv[1], "r");
	
	if (file == NULL) {
		fprintf(stderr, "Cannot open file: %s\n", argv[1]);
		return 2;
	}
	
	atts = read_ini_file(file);
	
	//just print the attributes on screen (test)
	if (atts != NULL) {
		for (i = 0 ; atts[i] != NULL ; i++) {
			att = atts[i];
		
			name = "";
			value = "";
			if (att->name != NULL)
				name = att->name;
			if (att->value != NULL)
				value = att->value;
		
			printf("%s -- %s\n", name, value);
		}
	}
	
	free_attributes(atts);
	fclose(file);
	return 0;
}
