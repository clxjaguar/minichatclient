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
#include "../ini.h"
#include "../clist.h"
#include "../attribute.h"

/**
 * Unit test for ini.
 */
int ini_test (int argc, char *argv[]) {
printf("INI_TEST DISABLED.\n");
return 0;
	clist *atts;
	FILE *file;
	clist_node *node;
	attribute *att;
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
	
	atts = ini_get_all(file, NULL);
	rewind(file);
	printf("%s -- %s\n", "key", ini_get(file, "key"));
	
	//just print the attributes on screen (test)
	for (node = atts->first ; node != NULL ; node = node->next) {
		att = (attribute *)node->data;
		
		name = "";
		value = "";
		if (att->name != NULL)
			name = att->name;
		if (att->value != NULL)
			value = att->value;
	
		printf("%s -- %s\n", name, value);
	}
	
	clist_free(atts);
	fclose(file);
	return 0;
}
