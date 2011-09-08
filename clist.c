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

clist *new_clist() {
	clist *list;
	
	list = (clist *)malloc(sizeof(clist));
	list->first = NULL;
	list->last = NULL;
	list->size = 0;
	
	return list;
}

clist_node *new_clist_node() {
	clist_node *node;
	
	node = (clist_node *)malloc(sizeof(clist_node));
	node->prev = NULL;
	node->next = NULL;
	node->data = NULL;
	node->free_node = NULL;
	
	return node;
}

void clist_add(clist *list, clist_node *node) {
	if (list->first == NULL) {
		list->first = node;
	}
	
	if (list->last != NULL) { 
		list->last->next = node;
	}
	
	node->prev = list->last;
	list->last = node;
	node->next = NULL;
	
	list->size++;
}


void clist_insert(clist *list, clist_node *node) {
	if (list->last == NULL) {
		list->last = node;
	}
	
	if (list->first != NULL) { 
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
	
	for (ptr = list->first ; ptr != NULL ; ) {
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
	if (node->prev == NULL) {
		list->first = node->next;
	} else {
		node->prev->next = node->next;
	}

	if (node->next == NULL) {
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
	if (anchor == NULL) {
		clist_insert(list, node);
	} else {
		node->prev = anchor->prev;
		anchor->prev = node;
		if (node->prev != NULL) {
			node->prev->next = node;
		}
		node->next = anchor;
		list->size++;
	}
}

void clist_insert_after(clist *list, clist_node *anchor, clist_node *node) {
	if (anchor == NULL) {
		clist_add(list, node);
	} else {
		node->next = anchor->next;
		anchor->next = node;
		if (node->next != NULL) {
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
	for (ptr = list->first ; ptr != NULL ; ptr = ptr->next) {
		if (indexi == i)  {
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
	for (ptr = list->first ; !inserted && ptr != NULL ; ptr = ptr->next) {
		if (indexi == i - 1)  {
			clist_insert_after(list, ptr, node);
			inserted = 1;
			break;
		}
		i++;
	}
	
	return inserted;
}

void free_clist_node(clist_node *node) {
	if (node->free_node != NULL) {
		node->free_node(node);
	} else {
		free(node);
	}
}

void free_clist_node_data(clist_node *node) {
	if (node->data != NULL) {
		free(node->data);
	}

	free(node);
}

void free_clist(clist *list) {
	clist_node *ptr;
	clist_node *next;
	
	ptr = list->first;
	while (ptr != NULL) {
		next = ptr->next;
		free_clist_node(ptr);
		ptr = next;
	}
	
	free(list);
}

