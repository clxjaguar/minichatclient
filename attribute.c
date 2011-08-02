/*
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

void free_attribute_node(clist_node *node);

attribute *new_attribute() {
	attribute *att;
	
	att = (attribute *)malloc(sizeof(attribute));
	att->name = NULL;
	att->value = NULL;
	
	return att;
}

clist_node *new_attribute_node() {
	clist_node *node;
	
	node = new_clist_node();
	node->free_node = free_attribute_node;
	
	return node;
}

void free_attribute(attribute *att) {
	if (att->name != NULL) {
		free (att->name);
	}
	if (att->value != NULL) {
		free (att->value);
	}
	free(att);
}

void free_attribute_node(clist_node *node) {
	attribute *att;
	
	if (node->data != NULL) {
		att = (attribute *)node->data;
		free_attribute(att);
	}
	
	free(node);
}

clist_node *attribute_add_to_clist(clist *list, attribute *att) {
	clist_node *node;
	
	node = new_attribute_node();
	node->data = att;
	clist_add(list, node);
	
	return node;
}

