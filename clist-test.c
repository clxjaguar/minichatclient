#include <stdlib.h>
#include <stdio.h>
#include "clist.h"

void print_list(clist *list) {
	clist_node *node;
	
	printf("Printing list:\n");
	for (node = list->first ; node != NULL ; node = node->next) {
		printf("%s\n", node->data);
	}
}

int main (int argc, char **argv) {
	clist *list;
	clist_node *node;
	
	list = new_clist();
	node = new_clist_node();
	node->data = "1";
	clist_add(list, node);
	node = new_clist_node();
	node->data = "2";
	clist_add(list, node);
	node = new_clist_node();
	node->data = "3";
	clist_add(list, node);
	node = NULL;
	
	print_list(list);
	printf("---\nReverse list...\n---\n");
	clist_reverse(list);
	print_list(list);
	
	free_clist(list);
	
	return 0;
}
