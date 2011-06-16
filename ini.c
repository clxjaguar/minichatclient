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
#include "ini_p.h"
#include "cstring.h"

attribute_t **read_ini_file(FILE *file) {
	attribute_t *att;
	attribute_t **atts;
	char buffer[81];
	size_t size;
	int full_line;
	cstring *string;
	
	atts = NULL;
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
				if (att->name[0] != '#')
					atts = attribute_add_to_list(atts, att);
				else
					free_attribute(att);
			}
			//
		}
	}
	
	free_cstring(string);
	return atts;
}

attribute_t *get_attribute(char data[]) {
	int i, len;
	cstring *key, *value;
	attribute_t *att;
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
			cstring_adds(value, (data + (i+1)));
		} else {
			key = new_cstring();
			cstring_adds(key, data);
		}
		
		att = malloc(sizeof(attribute_t));
		att->name = cstring_convert(key);
		att->value = cstring_convert(value);
	}
	
	return att;
}

void free_attribute(attribute_t* attribute) {
	if (attribute->name != NULL)
		free(attribute->name);
	if (attribute->value != NULL)
		free(attribute->value);
	
	free(attribute);
}

void free_attributes(attribute_t** attributes) {
	int i, ii;
	
	if (attributes != NULL) {
		for (i = 0 ; attributes[i] != NULL ; i++) {
			attribute_t *attribute = attributes[i];
			free_attribute(attribute);
		}
	}
	
	free(attributes);
}

attribute_t **attribute_add_to_list(attribute_t **list, attribute_t *att) {
	int i;
	attribute_t *ptr;
	
	ptr = NULL;
	i = 0;
	
	if (list != NULL)
		for (ptr = list[i] ; ptr != NULL ; ptr = list[++i]);
	else {
		list = malloc(sizeof(attribute_t *));
		list[0] = NULL;
	}
	
	list[i] = att;
	list = (attribute_t **)realloc(list, sizeof(attribute_t *) * (i + 2));
	list[i + 1] = NULL;
	
	return list;
}

void remove_crlf(char data[], int size) {
	// do not store the \n, convert CRLF to LF
	if (size > 0 && data[size-1] == '\n') {
		data[size-1] = '\0';
		if (size > 1 && data[size-2] == '\r')
			data[size-2] = '\0';
	}
}
