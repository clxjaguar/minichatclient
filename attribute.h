#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "clist.h"

typedef struct attribute_struct attribute;
struct attribute_struct {
	char *name;
	char *value;
};

attribute *new_attribute();
/**
 * Freeing the attribute will also free its name and value if they are not NULL.
 * If you don't want that, just call free() instead of free_attribute().
 */
void free_attribute(attribute *att);
void free_attributes(clist *atts);

#endif
