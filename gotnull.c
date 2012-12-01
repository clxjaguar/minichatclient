#ifndef WIN32
#include <unistd.h>
#endif

char* display_driver(void) {
	#ifdef WIN32
	Sleep(250);
	#else
	usleep(250 * 1000);
	#endif
	
	return NULL;
}

// others interfaces fonctions

void display_init(void) {
}

void display_debug(const char *text, int nonewline) {
}

void display_statusbar(const char *text) {
}

void display_conversation(const char *text) {
}

void display_nicklist(char *text) {
}

void display_end(void) {
}

char display_waitforchar(const char *msg) {
}