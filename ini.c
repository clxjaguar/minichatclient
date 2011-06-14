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

attribute_t **read_ini_file(FILE *file) {
	attribute_t **atts;
	char buffer[80];
	int size;
	char *tmp;
	int alloc;
	
	atts = NULL;
	
	if (file != NULL) {
		while (!feof(file)) {
			fgets(buffer, 80, file);
			size = strlen(buffer);
			// We got the whole content in one pass
			if (feof(file) || buffer[size-1] == '\n') {
				alloc = 0;
				tmp = buffer;
			}
			// No luck, we need to continue getting data
			else {
				alloc = 1;
				tmp = strcpy(tmp, buffer);
				//TODO!!
			}
			
			// get the attribute, and add it if it is not a comment
			attribute_t *att = get_attribute(tmp);
			if (att != NULL) {
				if (att->name[0] != '#')
					atts = attribute_add_to_list(atts, att);
				else
					free_attribute(att);
			}
			//
			
			if (alloc)
				free(tmp);
		}
	}
	
	return atts;
}

attribute_t *get_attribute(char data[]) {
	int i, len;
	char *key, *value, *tmp;
	attribute_t *att;
	
	for (i = 0 ; data[i] != '\0' && data[i] != '=' && data[i] != '\n' ; i++);
	
	if (data[i] == '=') {
		key = malloc(sizeof(char) * (i + 1));
		strncpy(key, data, i);
		key[i] = '\0';
		
		tmp = data + (i + 1);
		// discard the ending line if found
		len = strlen(tmp);
		if (tmp[len - 1] == '\n')
			len--;
		//
		value = malloc(sizeof(char) * (len + 1));
		strncpy(value, tmp, len);
		value[len] = '\0';
	} else {
		// discard the ending line if found
		len = strlen(data);
		if (data[len - 1] == '\n')
			len--;
		//
		
		if (len > 0) {
			key = malloc(sizeof(char) * (len + 1));
			strncpy(key, data, len);
			key[len] = '\0';
		} else {
			key = NULL;
		}
		
		value = NULL;
	}
	
	if (key != NULL) {
		att = malloc(sizeof(attribute_t));
		att->name = key;
		att->value = value;
	} else {
		att = NULL;
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
