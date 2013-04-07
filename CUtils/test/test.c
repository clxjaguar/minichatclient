#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "test.h"
#include "../cstring.h"
#include "../clist.h"

#include "cstring-test.c"
#include "clist-test.c"
#include "ini-test.c"
#include "net-test.c"
#include "htmlnode-test.c"

int main(int argc, char *argv[]) {
	int ret = 0;
	
	if (argc > 1) {
		printf("This programme does not support argument.\nYou passed: %s\n",
				argv[1]);
		return 1;
	}
	
	ret += cstring_test(argc, argv);
	ret += clist_test(argc, argv);
	//ret += net_test(argc, argv);
	
	char *tab[2];
	tab[0] = "./cutils-test";
	tab[1] = "test/test.ini";
	
	ret += ini_test(2, tab);
	ret += htmlnode_test(argc, argv);
	
	return ret;
}

#define boolean int
#define true 1
#define false 0

boolean uutest(const char *name, const char *expected, const char *value,
		boolean verbose, boolean invert);

boolean uutest_i(const char *name, int expected, int value, boolean verbose, boolean invert);

boolean test(const char *name, const char *expected, const char *value,
		boolean verbose) {
	return uutest(name, expected, value, verbose, false);
}

boolean test_i(const char *name, int expected, int value, boolean verbose) {
	return uutest_i(name, expected, value, verbose, false);
}

boolean ntest(const char *name, const char *expected, const char *value,
		boolean verbose) {
	return uutest(name, expected, value, verbose, true);
}

boolean ntest_i(const char *name, int expected, int value, boolean verbose) {
	return uutest_i(name, expected, value, verbose, true);
}


boolean uutest(const char *name, const char *expected, const char *value,
		boolean verbose, boolean invert) {
	boolean result = true;

	if (verbose) {
		printf("*** %25.25s     ..........     ", name);
		fflush(NULL);
	}

	result = !strcmp(value, expected);
	if (invert)
		result = !result;

	if (verbose) {
		printf((result ? "OK\n" : "KO\n"));
		if (!result) {
			if (invert)
				printf("'%s' should NOT have been '%s'\n", value, expected);
			else
				printf("'%s' should have been '%s'\n", value, expected);
		}
		fflush(NULL);
	}

	return result;
}

boolean uutest_i(const char *name, int expected, int value, boolean verbose, boolean invert) {
	boolean result = true;

	if (verbose) {
		printf("*** %25.25s     ..........     ", name);
		fflush(NULL);
	}

	result = (value == expected);
	if (invert)
		result = !result;

	if (verbose) {
		printf((result ? "OK\n" : "KO\n"));
		if (!result) {
			if (invert)
				printf("'%i' should NOT have been '%i'\n", value, expected);
			else
				printf("'%i' should have been '%i'\n", value, expected);
		}
		fflush(NULL);
	}

	return result;
}

