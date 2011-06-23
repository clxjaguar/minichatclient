#include <stdlib.h>
#include <stdio.h>
#include "clist.h"

void print_list(clist *list) {
	clist_node *node;
	
	printf("Printing list:\n{\n");
	for (node = list->first ; node != NULL ; node = node->next) {
		printf(" %s\n", node->data);
	}
	printf("}\n");
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
	printf("Reverse list...\n");
	clist_reverse(list);
	print_list(list);
	printf("Remove last node...\n");
	node = clist_remove(list, list->last);
	print_list(list);
	
	free_clist(list);
	free_clist_node(node);
	
	return 0;
}
