/**
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

#define bool int
#define true 1
#define false 0

// start of privates prototypes

/**
 * Create an attribute from a string (it will cut it in half at the sign '='
 * if found, or just put the whole string in the 'name' field).
 * It will return NULL if the given string describes a comment.
 * Data *MUST* end with '\0', or be NULL (which will return NULL).
 *
 * @param data the attribute in its text form
 *
 * @return the attribute, or NULL
 */
attribute *get_attribute(char data[]);

/**
 * Remove a trailing LN, or CRLN, and replace it by '\0'.
 */
void remove_crlf(char data[], size_t length);

bool match(attribute *att, void *argument);

// end of privates prototypes


char *ini_get(FILE *file, char name[]) {
	clist *list;
	attribute *att;
	cstring *out;
	
	out = NULL;
	list = ini_get_all(file, name);
	
	if (list->last != NULL) {
		out = cstring_new();
		att = (attribute *)list->last->data;
		cstring_adds(out, (att)->value);
	}
	
	free(list);
	return cstring_convert(out);
}

clist *ini_get_all(FILE *file, char mask[]) {
	return ini_get_select(file, match, mask);
}

bool match(attribute *att, void *argument) {
	//TODO: allow real masks instead of exact match?
	if (argument == NULL) {
		return true;
	} else {
		if (att == NULL || att->name == NULL) {
			return false;
		} else {
			return !strcmp(att->name, (char *)argument);
		}
	}
}

clist *ini_get_select(FILE *file, bool (*filter)(attribute *att, void *argument), void *argument) {
	clist *atts;
	attribute *att;
	char buffer[81];
	size_t size;
	bool full_line;
	cstring *string;
	
	atts = clist_new();
	string = cstring_new();
	buffer[80] = '\0'; // just in case
	
	if (file != NULL) {
		while (!feof(file)) {
			buffer[0]='\0';
			fgets(buffer, 80, file);
			// Note: strlen() could return 0 if the file contains \0
			// at the start of a line
			size = strlen(buffer);
			full_line = (feof(file) || size == 0 || buffer[size - 1] == '\n');
			remove_crlf(buffer, size);
			
			// No luck, we need to continue getting data
			if (!full_line) {
				cstring_clear(string);
				cstring_adds(string, buffer);
				while (!full_line) {
					fgets(buffer, 80, file);
					size = strlen(buffer);
					full_line = (feof(file) || size == 0 || buffer[size - 1] == '\n');
					remove_crlf(buffer, size);
					cstring_adds(string, buffer);
				}
				full_line = false;
			}
			
			// get the attribute, and add it if it is not a comment
			if (full_line) {
				att = get_attribute(buffer);
			} else {
				att = get_attribute(string->string);
			}
			
			if (att != NULL) {
				if (att->name[0] != '#' && att->name[0] != '\0' && (filter == NULL || filter(att, argument))) {
					attribute_add_to_clist(atts, att);
				} else {
					attribute_free(att);
				}
			}
			//
		}
	}
	
	cstring_free(string);
	return atts;
}

attribute *get_attribute(char data[]) {
	size_t i;
	cstring *key, *value;
	cstring *tmp;
	attribute *att;
	
	// Validity check.
	if (data == NULL) {
		return NULL;
	}
	
	att = NULL;
	key = NULL;
	value = NULL;
	
	if (strlen(data) > 0) {
		for (i = 0 ; data[i] != '\0' && data[i] != '=' ; i++);
		
		if (data[i] == '=') {
			key = cstring_new();
			cstring_addns(key, data, i);
			value = cstring_new();
			cstring_adds(value, (data + (i + 1)));
		} else {
			key = cstring_new();
			cstring_adds(key, data);
		}
		
		// trim
		tmp = key;
		key = cstring_trimc(key, ' ', true, true);
		cstring_free(tmp);
		
		// TODO: trim or not to trim?
		if (false && value != NULL) {
			tmp = value;
			value = cstring_trimc(tmp, ' ', true, true);
			cstring_free(tmp);
		}
		//
		
		att = attribute_new();
		att->name = cstring_convert(key);
		// Note: cstring_convert will return NULL if NULL is given
		att->value = cstring_convert(value);
	}
	
	return att;
}

void remove_crlf(char data[], size_t size) {
	// convert CRLF to LF, do not store the final \n
	if (size > 0 && data[size - 1] == '\n') {
		data[size - 1] = '\0';
		if (size > 1 && data[size - 2] == '\r')
			data[size - 2] = '\0';
	}
}
