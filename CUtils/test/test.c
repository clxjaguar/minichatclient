#include <stdio.h>
#include <unistd.h>

#include "cstring-test.c"
#include "clist-test.c"
#include "ini-test.c"
#include "net-test.c"

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
	
	return ret;
}
