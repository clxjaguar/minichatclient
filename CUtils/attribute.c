/**
  Name:        attribute.c
  Copyright:   niki (cc-by-nc) 2011
  Author:      niki
  Date:        2011-06-21
  Description:
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "clist.h"
#include "attribute.h"

void attribute_node_free(clist_node *node);

// Old
attribute *new_attribute() {
	return attribute_new();
}
clist_node *new_attribute_node() {
	return attribute_node_new();
}
void free_attribute(attribute *att) {
	attribute_free(att);
}
//

attribute *attribute_new() {
	attribute *att;

	att = malloc(sizeof(attribute));
	att->name = NULL;
	att->value = NULL;

	return att;
}

clist_node *attribute_node_new() {
	clist_node *node;

	node = clist_node_new();
	node->free_data = attribute_free;

	return node;
}

void attribute_free(attribute *att) {
	if (att->name != NULL) {
		free (att->name);
	}
	if (att->value != NULL) {
		free (att->value);
	}
	free(att);
}

clist_node *attribute_add_to_clist(clist *list, attribute *att) {
	clist_node *node;

	node = attribute_node_new();
	node->data = att;
	node->free_data = attribute_free;
	clist_add(list, node);

	return node;
}

