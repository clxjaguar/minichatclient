#ifndef WIN32
#include <unistd.h>
#endif

#include "commons.h"

char* display_driver(void) {
#ifdef WIN32
	Sleep(WAITING_TIME_GRANOLOSITY);
#else
	usleep(WAITING_TIME_GRANOLOSITY * 1000);
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

void display_nicklist(const char *text) {
}

void display_end(void) {
}

char display_waitforchar(const char *msg) {
}
