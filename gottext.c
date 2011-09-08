#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "cstring.h"
#include "commons.h"
#include "display_interfaces.h" // prototypes of theses display_* fonctions

int pipin = 0;
int stop = 0;
void display_init() {
	int pipefd[2];
	int car;

	//TODO: check return code
	pipe(pipefd);
	fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
	fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

	if (fork()) {
		// Parent
		pipin = pipefd[0];
	} else {
		// Child
		car = getc(stdin);
		while (!stop) {
			if (car > 0) {
				write(pipefd[1], &car, sizeof(char));
				car = getc(stdin);
			} else {
				stop = 1;
			}
		}
		
		printf("Input is DONE!\n");
		exit(0);
	}
}

void display_debug(const char *text, int nonewline) {
	if (nonewline) {
		fprintf(stderr, "%s", text);
	} else {
		fprintf(stderr, "\n%s", text);
	}
}

void display_statusbar(const char* message) {
	int needless = 1;
	if (message != NULL) needless = needless + 1;
}

void display_conversation(const char *text) {
	printf("%s\n", text);
}

void display_nicklist(char *text[], unsigned int nbrofnicks) {
	if (text != NULL) nbrofnicks = nbrofnicks;
}

void display_end() {
	//TODO: this is stupid with a fork...
	stop = 1;
	ungetc('\0', stdin);
}

char display_waitforchar(const char *msg) {
	int a = 1;
	if (msg != NULL) a = a + 1;

	return 'y';
}

/* include file for gotcurses.c and possibles others display interfaces
   for the minichatclient project.

   if something has been written from the keyboard, that fonction malloc 
   and return a pointer, else it returns NULL. Please do not free() on 
   what have been returned, as the fonction does it itself on the next call.
   Oh, and it also take care of the 250ms delay before returning. */

cstring *read_buffer = NULL;
cstring *send_buffer = NULL;
char* display_driver() {
	char enter_chars[] = { 0x0a, 0x0d, '\0' };
	char buffer[11];
	ssize_t size;
	long long enter;
	cstring *tmp;

	if (read_buffer == NULL) {
		read_buffer = new_cstring();
	}

	if (send_buffer == NULL) {
		send_buffer = new_cstring();
	} else {
		cstring_clear(send_buffer);
	}

	// reads everything into string
	buffer[10] = '\0';
	size = 1;
	while (size > 0) {
		// can return -1 and error set to EAGAIN
		size = read(pipin, buffer, 10);
		if (size > 0) {
			buffer[size] = '\0';
			cstring_addns(read_buffer, buffer, size);
		}
	}
	//
	
	/*if(read_buffer->length > 0) {
		size_t iii = 0;
		printf("chars:");
		for (iii = 0 ; iii < read_buffer->length ; iii++) {
			printf(" %i", read_buffer->string[iii]);
		}
		printf("\n");
	}*/
	
	enter = cstring_find_anys(read_buffer, enter_chars, 0);
	if (enter > 0) {
		if ((size_t)enter + 1 != read_buffer->length) {
			cstring_addf(send_buffer, read_buffer, (size_t)enter + 1);
			cstring_cut_at(read_buffer, (size_t)enter);
		} else {
			cstring_cut_at(read_buffer, (size_t)enter);
		}
		tmp = send_buffer;
		send_buffer = read_buffer;
		read_buffer = tmp;

		return send_buffer->string;
	} else {
		usleep(WAITING_TIME_GRANOLOSITY * 1000);
		return NULL;
	}

	//TODO:
	int ch = fgetc(stdin);
	switch (ch){
	case 0x0a: // "enter" key
	case 0x0d: // also enter, in some OSes
		//return buffer;
		break;

	case 0x08: // ascii backspace
	case 0x7f: //  "^?" backspace
	
	default:
	//	cstring_addc(string, ch);
	break;
	}
	
	return NULL;
	//WAITING_TIME_GRANOLOSITY
}
