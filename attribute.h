#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "clist.h"

/**
 * This is an attribute (a key-value pair).
 * Note that the name cannot be NULL, but the value can.
 */
typedef struct attribute_struct attribute;
struct attribute_struct {
	char *name;
	char *value;
};

attribute *new_attribute();

/**
 * Create a new clist_node that will contain an attribute,
 * and will free it's data (both name and value, if not NULL) when free_clist is called.
 * If you don't want that, just set the given clist_node->free_node to NULL.
 */
clist_node *new_attribute_node();

void free_attribute(attribute *att);

clist_node *attribute_add_to_clist(clist *list, attribute *att);

#endif
