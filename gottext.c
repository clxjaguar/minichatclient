#ifndef WIN32
#include <unistd.h>
#endif

#include "CUtils/net.h"
#include "CUtils/cstring.h"

#include "display_interfaces.h" // prototypes of theses display_* fonctions
#include "commons.h"

cstring *gottext_buffer = NULL;

const char* display_driver(void) {
#ifdef WIN32
	Sleep(WAITING_TIME_GRANOLOSITY);
#else
	usleep(WAITING_TIME_GRANOLOSITY * 1000);
#endif

	if (!gottext_buffer)
		gottext_buffer = cstring_new();
	
	cstring_readline(gottext_buffer, stdin);

	if (gottext_buffer->length){
		return gottext_buffer->string;
		// TODO: hey le roo, t'es sur que tu nous fais pas une fuite de memoire la ? tu dois free au prochain appel !
		// TODO: hey le chat, oui je suis sûr ? Je garde le même buffer durant toute la durée du programme (toujours pour la même raison, je n'étais pas sûr de ce qui allait advenir du buffer -- je déteste renvoyer une zone mémoire et de malgré tout devoir la gérer -- donc je la recycle)
	}
	return NULL;
}

// others interfaces fonctions

void display_init(void) {
	net_set_blocking(fileno(stdin), 0);
}

void display_debug(const char *text, int nonewline) {
	fprintf(stderr, "%s", text);
	if (!nonewline){
		fprintf(stderr, "\n");
	}
}

void display_statusbar(const char *text) {
	if (text){}
}

void display_conversation(const char *text) {
	printf("%s\n", text);
}

void display_nicklist(const char *text) {
	if (text){}
}

void display_end(void) {
	if (gottext_buffer){
		cstring_free(gottext_buffer);
	}
	gottext_buffer = NULL;

	net_set_blocking(fileno(stdin), 1);
}

char display_waitforchar(const char *msg) {
	if (msg){}
	return '\0';
}
