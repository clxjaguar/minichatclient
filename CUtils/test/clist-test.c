#include <stdlib.h>
#include <stdio.h>
#include "../clist.h"

void print_list(clist *list) {
	clist_node *node;

	printf("Printing list:\n{\n");
	for (node = list->first; node != NULL; node = node->next) {
		printf(" %s\n", (char *) node->data);
	}
	printf("}\n");
}

int clist_test(int argc, char **argv) {
	if (argc > 1) {
		printf("This programme does not support argument.\nYou passed: %s\n",
				argv[1]);
		return 1;
	}

	clist *list;
	clist_node *node;

	list = clist_new();
	node = clist_node_new();
	node->data = "1";
	clist_add(list, node);
	node = clist_node_new();
	node->data = "2";
	clist_add(list, node);
	node = clist_node_new();
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

	clist_free(list);
	clist_node_free(node);

	printf("New List...\n");
	list = clist_new();
	node = clist_node_new();
	node->data = "3";
	clist_add(list, node);
	node = NULL;
	print_list(list);
	printf("Remove last node...\n");
	node = clist_remove(list, list->last);
	print_list(list);

	clist_free(list);
	clist_node_free(node);

	return 0;
}
