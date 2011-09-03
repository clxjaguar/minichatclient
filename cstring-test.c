#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cstring.h"
#include "clist.h"

#define boolean int
#define true 1
#define false 0

boolean test(const char *name, const char *expected, const char *value, boolean verbose);
boolean test_i(const char *name, int expected, int value, boolean verbose);
boolean do_operations(boolean verbose);

boolean test(const char *name, const char *expected, const char *value, boolean verbose) {
	boolean result = true;
	
	if (verbose) {
		printf("*** %25.25s     ..........     ", name);
		fflush(NULL);
	}
	
	result = !strcmp(value, expected);
	
	if (verbose) {
		printf((result ? "OK\n" : "KO\n"));
		if (!result) {
			printf("'%s' should have been '%s'\n", value, expected);
		}
		fflush(NULL);
	}
	
	return result;
}

boolean test_i(const char *name, int expected, int value, boolean verbose) {
	boolean result = true;
	
	if (verbose) {
		printf("*** %25.25s     ..........     ", name);
		fflush(NULL);
	}
	
	result = (value == expected);
	
	if (verbose) {
		printf((result ? "OK\n" : "KO\n"));
		if (!result) {
			printf("'%i' should have been '%i'\n", value, expected);
		}
		fflush(NULL);
	}
	
	return result;
}

int main (int argc, char **argv) {
	unsigned long i;
	boolean result;
	
	if (argc > 1) {
		printf("This programme does not support argument.\nYou passed: %s\n", argv[1]);
		return 1;
	}

	result = do_operations(true);

	if (result) {
		// Check for memory leaks/check perfs
		printf("*** %25.25s     ..........     ", "Running 100.000 operations");
		fflush(NULL);
		for (i = 0 ; result && i < 100 * 1000 ; i++ ){
			result = result && do_operations(false);
		}
		printf((result ? "OK\n" : "KO\n"));
	}
	
	return !result;
}

boolean do_operations(boolean verbose) {
	cstring *string, *string2, *string3;
	char *s2;
	clist *split_list;
	boolean b;
	boolean result = true;
	clist_node *node;

	string = new_cstring();
	result = result && test("Empty string", "", string->string, verbose);
	cstring_reverse(string);
	result = result && test("Reverse empty string", "", string->string, verbose);
	cstring_adds(string, "Test.");
	result = result && test("adds", "Test.", string->string, verbose);
	cstring_clear(string);
	result = result && test("clear", "", string->string, verbose);
	cstring_adds(string, "0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 Test.");
	result = result && test("adds", "0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 Test.", string->string, verbose);
	cstring_reverse(string);
	result = result && test("reverse non-empty string", ".tseT 9876543210 9876543210 9876543210 9876543210 9876543210 9876543210 9876543210 9876543210", string->string, verbose);
	cstring_addi(string, 98);
	result = result && test("addi", ".tseT 9876543210 9876543210 9876543210 9876543210 9876543210 9876543210 9876543210 987654321098", string->string, verbose);
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

	result = result && test("addi/addc/adds", "String 2", string->string, verbose);
	result = result && test("char **conversions", "String 2", *(char **)string, verbose);

	cstring_clear(string);
	cstring_addns(string, "1234567890", 4);
	result = result && test("addns", "1234", string->string, verbose);
	cstring_clear(string);
	cstring_adds(string, "1234567890");
	cstring_cut_at(string, 4);
	result = result && test("cut_at", "1234", string->string, verbose);
	cstring_clear(string);
	cstring_addfs(string, "1234567890", 4);
	result = result && test("addfs", "567890", string->string, verbose);
	cstring_clear(string);
	cstring_addfns(string, "1234567890", 4, 4);
	result = result && test("addfns", "5678", string->string, verbose);
	cstring_clear(string);
	cstring_addi(string, 12);
	result = result && test("addi", "12", string->string, verbose);
	cstring_addx(string, 0xFF2);
	result = result && test("addx", "12ff2", string->string, verbose);
	cstring_clear(string);
	cstring_addX(string, 0xABAC);
	result = result && test("addX", "ABAC", string->string, verbose);
	cstring_reverse(string);
	result = result && test("reverse non-empty string", "CABA", string->string, verbose);
	cstring_clear(string);
	cstring_adds(string, "12 34  '56 78'");
	
	split_list = cstring_splitc(string, ' ', '\'');
	node = split_list->first;
	result = result && test("splitc 1", "12", ((cstring *)node->data)->string, verbose);
	node = node->next;
	result = result && test("splitc 1", "34", ((cstring *)node->data)->string, verbose);
	node = node->next;
	result = result && test("splitc 1", "", ((cstring *)node->data)->string, verbose);
	node = node->next;
	result = result && test("splitc 1", "56 78", ((cstring *)node->data)->string, verbose);
	result = result && test_i("splitc 1", true, node->next == NULL, verbose);
	
	free_clist(split_list);
	split_list = cstring_splitc(string, 'Z', '\0');
	node = split_list->first;
	result = result && test("splitc 2", "12 34  '56 78'", ((cstring *)node->data)->string, verbose);
	result = result && test_i("splitc 2", true, node->next == NULL, verbose);
	
	free_clist(split_list);
	cstring_clear(string);
	cstring_adds(string, "\"12=34\"=56=\"78=90\"");
	split_list = cstring_splitc(string, '=', '\"');
	node = split_list->first;
	result = result && test("splitc 3", "12=34", ((cstring *)node->data)->string, verbose);
	node = node->next;
	result = result && test("splitc 3", "56", ((cstring *)node->data)->string, verbose);
	node = node->next;
	result = result && test("splitc 3", "78=90", ((cstring *)node->data)->string, verbose);
	result = result && test_i("splitc 3", true, node->next == NULL, verbose);
	
	free_clist(split_list);
	cstring_clear(string);
	cstring_adds(string, "12345 67890");
	string2 = new_cstring();
	string3 = new_cstring();
	cstring_adds(string2, "78");
	cstring_adds(string3, "__");
	cstring_replace(string, string2, string3);
	result = result && test("replace", "12345 6__90", string->string, verbose);
	cstring_clear(string);
	cstring_clear(string2);
	cstring_clear(string3);
	cstring_adds(string, "12345 67890");
	cstring_adds(string2, "78");
	cstring_adds(string3, "_");
	cstring_replace(string, string2, string3);
	result = result && test("replace", "12345 6_90", string->string, verbose);
	cstring_clear(string);
	cstring_clear(string2);
	cstring_clear(string3);
	cstring_adds(string, "12345 67890");
	cstring_adds(string2, "78");
	cstring_adds(string3, "___");
	cstring_replace(string, string2, string3);
	result = result && test("replace", "12345 6___90", string->string, verbose);
	free_cstring(string2);
	free_cstring(string3);
	cstring_clear(string);
	cstring_adds(string, "1234567890");
	string2 = cstring_substring(string, 3, 2);
	result = result && test("replace", "45", string2->string, verbose);
	free_cstring(string2);
	cstring_clear(string);
	cstring_adds(string, "1234567890");
	b = cstring_starts_withs(string, "123", 0);
	result = result && test_i("starts_withs", true, b, verbose);
	b = cstring_ends_withs(string, "890", 0);
	result = result && test_i("ends_withs", true, b, verbose);
	b = cstring_starts_withs(string, "124, 0", 0);
	result = result && test_i("starts_withs", false, b, verbose);
	b = cstring_ends_withs(string, "8900", 0);
	result = result && test_i("ends_withs", false, b, verbose);
	cstring_clear(string);
	cstring_adds(string, " text text @@ ");
	b = cstring_ends_withs(string, "@ ", 0);
	result = result && test_i("ends_withs", true, b, verbose);
	cstring_clear(string);
	cstring_adds(string, "@ ");
	b = cstring_ends_withs(string, "@ ", 0);
	result = result && test_i("ends_withs", true, b, verbose);

	free_cstring(string);
	
	return result;
}
