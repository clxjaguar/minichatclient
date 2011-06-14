#ifndef INI_H
#define INI_H

#include <stdio.h>

/**
 * This is an attribute (a key-value pair).
 * You should use attribute_t instead of this struct.
 * Note that the name cannot be NULL, but the value can.
 */
struct attribute {
	char *name;
	char *value;
};

/**
 * This is an attribute (a key-value pair).
 */
typedef struct attribute attribute_t;

/**
 * Read the content of an INI file (only one level without [sections], comments starts with #, whole line only).
 */
attribute_t **read_ini_file(FILE *file);

/**
 * Free an attribute_t.
 */
void free_attribute(attribute_t* attribute);

/**
 * Free a list of attribute_t.
 */
void free_attributes(attribute_t ** attributes);

#endif
