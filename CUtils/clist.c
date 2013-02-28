/*
 Name:        clist.c
 Copyright:   niki (cc-by-nc) 2011
 Author:      niki
 Date:        2011-06-21
 Description:
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "clist.h"

// Old
clist *new_clist() {
	return clist_new();
}
clist_node *new_clist_node() {
	return clist_node_new();
}
void free_clist_node_data(clist_node *node) {
	if (!node)
		return;
	
	free(node->data);
}
void free_clist(clist *list) {
	clist_free(list);
}
void free_clist_node(clist_node *node) {
	clist_node_free(node);
}
//

clist *clist_new() {
	clist *list;

	list = (clist *) malloc(sizeof(clist));
	list->first = NULL;
	list->last = NULL;
	list->size = 0;

	return list;
}

clist_node *clist_node_new() {
	clist_node *node;

	node = (clist_node *) malloc(sizeof(clist_node));
	node->prev = NULL;
	node->next = NULL;
	node->data = NULL;
	node->free_data = NULL;
	node->free_content = NULL;

	return node;
}

void clist_add(clist *list, clist_node *node) {
	if (!list->first) {
		list->first = node;
	}

	if (list->last) {
		list->last->next = node;
	}

	node->prev = list->last;
	list->last = node;
	node->next = NULL;

	list->size++;
}

void clist_insert(clist *list, clist_node *node) {
	if (!list->last) {
		list->last = node;
	}

	if (list->first) {
		list->first->prev = node;
	}

	node->next = list->first;
	list->first = node;
	node->prev = NULL;

	list->size++;
}

void clist_reverse(clist *list) {
	clist_node *ptr;
	clist_node *next;

	for (ptr = list->first ; ptr ; ) {
		next = ptr->next;

		ptr->next = ptr->prev;
		ptr->prev = next;

		ptr = next;
	}

	next = list->first;
	list->first = list->last;
	list->last = next;
}

clist_node *clist_remove(clist *list, clist_node *node) {
	if (!node->prev) {
		list->first = node->next;
	} else {
		node->prev->next = node->next;
	}

	if (!node->next) {
		list->last = node->prev;
	} else {
		node->next->prev = node->prev;
	}

	node->prev = NULL;
	node->next = NULL;

	list->size--;

	return node;
}

void clist_insert_before(clist *list, clist_node *anchor, clist_node *node) {
	if (!anchor) {
		clist_insert(list, node);
	} else {
		node->prev = anchor->prev;
		anchor->prev = node;
		if (node->prev) {
			node->prev->next = node;
		}
		node->next = anchor;
		list->size++;
	}
}

void clist_insert_after(clist *list, clist_node *anchor, clist_node *node) {
	if (!anchor) {
		clist_add(list, node);
	} else {
		node->next = anchor->next;
		anchor->next = node;
		if (node->next) {
			node->next->prev = node;
		}
		node->prev = anchor;
		list->size++;
	}
}

clist_node *clist_get(clist *list, size_t indexi) {
	clist_node *ptr;
	size_t i;

	i = 0;
	for (ptr = list->first ; ptr ; ptr = ptr->next) {
		if (indexi == i) {
			return ptr;
		}
		i++;
	}

	return NULL;
}

int clist_insert_at(clist *list, size_t indexi, clist_node *node) {
	clist_node *ptr;
	int inserted;
	size_t i;

	// special case
	if (list->first == NULL && indexi == 0) {
		clist_insert(list, node);
		return 1;
	}

	inserted = 0;
	i = 0;
	for (ptr = list->first; !inserted && ptr ; ptr = ptr->next) {
		if (indexi == i - 1) {
			clist_insert_after(list, ptr, node);
			inserted = 1;
			break;
		}
		i++;
	}

	return inserted;
}

void clist_node_free(clist_node *node) {
	void (*free_data_function)(void*);

	if (!node)
		return;

	if (node->data) {
		if (node->free_content) {
			free_data_function = node->free_content;
			free_data_function(node->data);
		} else if (node->free_data) {
			free_data_function = node->free_data;
			free_data_function(node->data);
		}
	}

	free(node);
}

void clist_free(clist *list) {
	clist_node *ptr;
	clist_node *next;

	if (!list)
		return;

	ptr = list->first;
	while (ptr != NULL) {
		next = ptr->next;
		clist_node_free(ptr);
		ptr = next;
	}

	free(list);
}

