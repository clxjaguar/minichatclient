#include <stdio.h>
#include <stdlib.h>
#include "display_interfaces.h"
#include "cstring.h"

#define true 1
#define false 0

#define MAX 1000000

int main (int argc, char **argv) {
	// disable warnings
	if (argv == 0) argc = argc;

	size_t index;
	char *str;

printf("Will display...\n");
	display_init();
printf("Did display.\n");

	index = 0;
	while (index < MAX) {
		if (index % 4 == 0) {
			cstring *s = new_cstring();
			cstring_adds(s, "Conversation index step count: ");
			cstring_addi(s, index);
			display_conversation(s->string);
			free_cstring(s);
		}
		
		str = display_driver();
		if (str != NULL) {
			display_debug("SEND: ", 1);
			display_debug(str, 0);
		}
		index++;
	}
	return 0;
}
