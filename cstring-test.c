#include <stdlib.h>
#include <stdio.h>
#include "cstring.h"
#include "clist.h"

void do_operations(int verbose);
void print_cstring_list(clist *list);

void print_cstring_list(clist *list) {
	clist_node *node;
	
	printf("Printing list:\n{\n");
	for (node = list->first ; node != NULL ; node = node->next) {
		printf(" %s\n", ((cstring *)node->data)->string);
	}
	printf("}\n");
}

int main (int argc, char **argv) {
	unsigned long i;

	do_operations(1);

	// Check for memory leaks/check perfs
	printf("Running 100.000 operations...\n");
	for (i = 0 ; i < 100 * 1000 ; i++ ){
		do_operations(0);
	}
	printf("Done.\n");
	
	return 0;
}

void do_operations(int verbose) {
	cstring *string, *string2, *string3;
	char *s2;
	clist *split_list;
	int b;

	string = new_cstring();
	if (verbose) {
		printf("Empty cstring: '%s'\n", string->string);
	}
	cstring_reverse(string);
	if (verbose) {
		printf("Reverse (still empty): '%s'\n", string->string);
	}
	cstring_adds(string, "Test.");
	if (verbose) {
		printf("Add 'Test.': '%s'\n", string->string);
	}
	cstring_clear(string);
	if (verbose) {
		printf("Clear it (empty cstring): '%s'\n", string->string);
	}
	cstring_adds(string, "0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 Test.");
	if (verbose) {
		printf("Adds 8 times 0->9, space, then ' Test.': '%s'\n", string->string);
	}
	cstring_reverse(string);
	if (verbose) {
		printf("Reverse it: '%s'\n", string->string);
	}
	cstring_addi(string, 98);
	if (verbose) {
		printf("Add '98'  : '%s'\n", string->string);
	}
	free_cstring(string);
	
	string = new_cstring();
	
	string2 = new_cstring();
	cstring_adds(string2, "String");
	s2 = cstring_convert(string2);
	cstring_adds(string, s2);
	free(s2);
	string2 = new_cstring();
	cstring_addc(string2, ' ');
	cstring_addi(string2, 2);
	cstring_add(string, string2);
	free_cstring(string2);

	if (verbose) {
		printf("You should see 'String 2': '%s'\n", string->string);
		printf("You should see 'String 2': '%s'\n", *(char **)string);
	}

	cstring_clear(string);
	cstring_addns(string, "1234567890", 4);
	if (verbose) {
		printf("You should see 1234: '%s'\n", string->string);
	}
	cstring_clear(string);
	cstring_adds(string, "1234567890");
	cstring_cut_at(string, 4);
	if (verbose) {
		printf("You should see 1234: '%s'\n", string->string);
	}
	cstring_clear(string);
	cstring_addfs(string, "1234567890", 4);
	if (verbose) {
		printf("You should see 567890: '%s'\n", string->string);
	}
	cstring_clear(string);
	cstring_addfns(string, "1234567890", 4, 4);
	if (verbose) {
		printf("You should see 5678: '%s'\n", string->string);
	}
	cstring_clear(string);
	cstring_addi(string, 12);
	if (verbose) {
		printf("You should see 12: '%s'\n", string->string);
	}
	cstring_addx(string, 0xFF2);
	if (verbose) {
		printf("You should see 12ff2: '%s'\n", string->string);
	}
	cstring_clear(string);
	cstring_addX(string, 0xABAC);
	if (verbose) {
		printf("You should see ABAC: '%s'\n", string->string);
	}
	cstring_reverse(string);
	if (verbose) {
		printf("You should see CABA: '%s'\n", string->string);
	}
	cstring_clear(string);
	cstring_adds(string, "12 34  '56 78'");
	split_list = cstring_splitc(string, ' ', '\'');
	if (verbose) {
		printf("You should see 12, then 34, then '', then '56 78':\n");
		print_cstring_list(split_list);
	}
	free_clist(split_list);
	split_list = cstring_splitc(string, 'Z', '\0');
	if (verbose) {
		printf("You should see \"12 34  '56 78'\":\n");
		print_cstring_list(split_list);
	}
	free_clist(split_list);
	cstring_clear(string);
	cstring_adds(string, "12345 67890");
	string2 = new_cstring();
	string3 = new_cstring();
	cstring_adds(string2, "78");
	cstring_adds(string3, "__");
	cstring_replace(string, string2, string3);
	if (verbose) {
		printf("You should see '12345 6__90': '%s'\n", string->string);
	}
	cstring_clear(string);
	cstring_clear(string2);
	cstring_clear(string3);
	cstring_adds(string, "12345 67890");
	cstring_adds(string2, "78");
	cstring_adds(string3, "_");
	cstring_replace(string, string2, string3);
	if (verbose) {
		printf("You should see '12345 6_90': '%s'\n", string->string);
	}
	cstring_clear(string);
	cstring_clear(string2);
	cstring_clear(string3);
	cstring_adds(string, "12345 67890");
	cstring_adds(string2, "78");
	cstring_adds(string3, "___");
	cstring_replace(string, string2, string3);
	if (verbose) {
		printf("You should see '12345 6___90': '%s'\n", string->string);
	}
	free_cstring(string2);
	free_cstring(string3);
	cstring_clear(string);
	cstring_adds(string, "1234567890");
	string2 = cstring_substring(string, 3, 2);
	if (verbose) {
		printf("You should see '45': '%s'\n", string2->string);
	}
	free_cstring(string2);
	cstring_clear(string);
	cstring_adds(string, "1234567890");
	b = cstring_starts_withs(string, "123", 0);
	if (verbose) {
		printf("'1234567890' starts with '123': %s\n", (b?"YES":"NO"));
	}
	b = cstring_ends_withs(string, "890", 0);
	if (verbose) {
		printf("'1234567890' ends with '890': %s\n", (b?"YES":"NO"));
	}
	b = cstring_starts_withs(string, "124, 0", 0);
	if (verbose) {
		printf("'1234567890' starts with '124': %s\n", (b?"YES":"NO"));
	}
	b = cstring_ends_withs(string, "8900", 0);
	if (verbose) {
		printf("'1234567890' ends with '8900': %s\n", (b?"YES":"NO"));
	}
	cstring_clear(string);
	cstring_adds(string, " text text @@ ");
	b = cstring_ends_withs(string, "@ ", 0);
	if (verbose) {
		printf("'%s' ends with '@ ': %s\n", string->string, (b?"YES":"NO"));
	}

	free_cstring(string);
}
