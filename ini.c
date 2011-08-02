/*
  Name:        ini.c
  Copyright:   niki (cc-by-nc) 2011
  Author:      niki
  Date:        13/06/11 10:43
  Description: More or less manage an INI file and return a list of key/values
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ini.h"
#include "cstring.h"
#include "clist.h"
#include "attribute.h"

#include <stdio.h>
#include "attribute.h"

// start of privates prototypes
/**
 * Create an attribute from a string (it will cut it in half at the sign '=' if found,
 * or just put the whole string in the 'name' field).
 * It will return NULL if the given string describes a comment.
 */
attribute *get_attribute(char data[]);

/**
 * Remove a trailing LN, or CRLN, and replace it by '\0'.
 */
void remove_crlf(char data[], int length);

char *mmask;
int match(attribute *att);
// end of privates prototypes


char *ini_get(FILE *file, char name[]) {
	clist *list;
	attribute *att;
	cstring *out;
	
	out = NULL;
	list = ini_get_all(file, name);
	
	if (list->last != NULL) {
		out = new_cstring();
		att = (attribute *)list->last->data;
		cstring_adds(out, (att)->value);
	}
	
	free(list);
	return cstring_convert(out);
}

clist *ini_get_all(FILE *file, char mask[]) {
	clist *list;

	mmask = mask;
	list = ini_get_select(file, match);
	mmask = NULL;

	return list;
}

int match(attribute *att) {
	//TODO: allow real masks instead of exact match?
	if (mmask == NULL) {
		return 1;
	} else {
		return !strcmp(att->name, mmask);
	}
}

clist *ini_get_select(FILE *file, int (*filter)(attribute *att)) {
	clist *atts;
	attribute *att;
	char buffer[81];
	size_t size;
	int full_line;
	cstring *string;
	
	atts = new_clist();
	string = new_cstring();
	buffer[80] = '\0'; // just in case
	
	if (file != NULL) {
		while (!feof(file)) {
			buffer[0]='\0';
			fgets(buffer, 80, file);
			size = strlen(buffer);
			full_line = (feof(file) || buffer[size-1] == '\n');
			remove_crlf(buffer, size);
			
			// No luck, we need to continue getting data
			if (!full_line) {
				cstring_clear(string);
				cstring_adds(string, buffer);
				while (!full_line) {
					fgets(buffer, 80, file);
					size = strlen(buffer);
					full_line = (feof(file) || buffer[size-1] == '\n');
					remove_crlf(buffer, size);
					cstring_adds(string, buffer);
				}
				full_line = 0;
			}
			
			// get the attribute, and add it if it is not a comment
			if (full_line)
				att = get_attribute(buffer);
			else
				att = get_attribute(string->string);
			
			if (att != NULL) {
				if (att->name[0] != '#' && (filter == NULL || filter(att))) {
					attribute_add_to_clist(atts, att);
				}
				else {
					free_attribute(att);
				}
			}
			//
		}
	}
	
	free_cstring(string);
	return atts;
}

attribute *get_attribute(char data[]) {
	int i;
	cstring *key, *value;
	attribute *att;
	cstring *line;
	
	line = new_cstring();
	cstring_adds(line, data);
	att = NULL;
	key = NULL;
	value = NULL;
	
	if (line->length > 0) {
		for (i = 0 ; data[i] != '\0' && data[i] != '=' ; i++);
		
		if (data[i] == '=') {
			key = new_cstring();
			cstring_addns(key, data, i);
			value = new_cstring();
			cstring_adds(value, (data + (i + 1)));
		} else {
			key = new_cstring();
			cstring_adds(key, data);
		}
		
		att = new_attribute();
		att->name = cstring_convert(key);
		att->value = cstring_convert(value);
	}
	
	return att;
}

void remove_crlf(char data[], int size) {
	// do not store the \n, convert CRLF to LF
	if (size > 0 && data[size-1] == '\n') {
		data[size-1] = '\0';
		if (size > 1 && data[size-2] == '\r')
			data[size-2] = '\0';
	}
}
