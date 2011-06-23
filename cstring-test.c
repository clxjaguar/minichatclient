#include <stdlib.h>
#include <stdio.h>
#include "cstring.h"

int main (int argc, char **argv) {
	unsigned long i;
	cstring *string, *string2;
	
	// Check for memory leaks/check perfs
	printf("Running 100.000 operations...\n");
	for (i = 0 ; i < 100 * 1000 ; i++ ){
		string = new_cstring();
		cstring_reverse(string);
		cstring_adds(string, "Test.");
		cstring_clear(string);
		cstring_adds(string, "0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 Test.");
		cstring_reverse(string);
		cstring_addi(string, 98);
		free_cstring(string);
	}
	printf("Done.\n");
	
	string = new_cstring();
	
	string2 = new_cstring();
	cstring_adds(string2, "String 2");
	cstring_add(string, string2);
	free_cstring(string2);
	printf("You should see String 2: %s\n", string->string);
	printf("You should see String 2: %s\n", *(char **)string);
	
	cstring_clear(string);
	cstring_addns(string, "1234567890", 4);
	printf("You should see 1234: %s\n", string->string);
	cstring_clear(string);
	cstring_addi(string, 12);
	printf("You should see 12: %s\n", string->string);
	cstring_addx(string, 0xFF2);
	printf("You should see 12ff2: %s\n", string->string);
	cstring_clear(string);
	cstring_addX(string, 0xABAC);
	printf("You should see ABAC: %s\n", string->string);
	cstring_reverse(string);
	printf("You should see CABA: %s\n", string->string);
	free_cstring(string);
	
	return 0;
}
