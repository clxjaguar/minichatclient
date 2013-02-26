/*
  Name:         conf.c
  Author:       cLx
  Date:         21/06/11
  Description:  Simple enough solution for reading a configuration file
  Copyright:    CC-BY-NC 2011
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "display_interfaces.h"

#define CONFIGURATION_FILE      "mchatclient.conf"
#define KEY_VALUE_BUFFER_SIZE   200

typedef enum {
	false = 0,
	true = 1
} bool;

typedef enum {
	VRFY_KEY = 0,
	WF_VALUE,
	WF_NEWLINE,
	IN_VALUE,
	END
} tstate;

static FILE *fd = NULL;

char *read_conf_string(const char *key, char *pvalue, size_t valuebufsize) {
	tstate state;
	unsigned int i, o;
	char buf[KEY_VALUE_BUFFER_SIZE];
	char c;
	bool lastpass = false;

	if (!key || !key[0]) {
		display_debug("*read_conf_string() : you must specify the key's name !", 0);
		return NULL;
	}

	if (!fd) {
		display_debug("Opening the configuration file...", 0);

		fd = fopen(CONFIGURATION_FILE, "r");
		if (!fd) {
			display_debug("Unable to open "CONFIGURATION_FILE" !\a", 0);
			return NULL;
		}
		lastpass = true;
	}

	state = VRFY_KEY; i = 0; o = 0;
	while(state != END) {
		c = (char)fgetc(fd);

		if (feof(fd)) {
			if (!lastpass) {
				if (state == IN_VALUE) {
					state = END;
				}
				else {
					fseek(fd, 0, SEEK_SET);
					lastpass = true;
					state = VRFY_KEY; i = 0;
				}
			}
			else {
				state = END;
				break;
			}
		}

		switch(state){
			default:
			case VRFY_KEY:
				if (!i && (c == '#' || c == ';')) {
					// comment => ignore.
					state = WF_NEWLINE;
				}
				else if (c == '\r' || c == '\n') {
					//newline ? oh, ok, reset the search.
					i = 0;
				}
				else if (c == ' ' || c == '\t' || c == '='){
					// separator?
					if (!i) { break; }
					if (!key[i]) { state = WF_VALUE; }
					else { state = WF_NEWLINE; }
				}
				else if (c != key[i]) {
					// now we see it's not the good key
					state = WF_NEWLINE;
				}
				else {
					// yet seem the good key
					i++;
				}
				break;

			case WF_VALUE:
				if (c == '\r' || c == '\n') {
					//newline ?
					i = 0;
					state = VRFY_KEY;
				}
				else if (c != ' ' && c != '\t'){
					// ok it's not a separator anymore
					o = 0;
					buf[o++] = c;
					state = IN_VALUE;
				}
				break;

			case WF_NEWLINE:
				if (c == '\r' || c == '\n') {
					state = VRFY_KEY;
					i = 0;
				}
				break;

			case IN_VALUE:
				if (c == '\r' || c == '\n' || c == '\t') {
					state = END;
					break;
				}
				else {
					buf[o++] = c;
					if (o>=sizeof(buf)) {
						display_debug("Buffer full reading value of :", 0);
						display_debug(key, 1);
						state = END;
					}
				}
				break;

			case END:
				//do nothing more.
				break;
		}
	}
	buf[o] = '\0';

	if (o) {
		if (pvalue && valuebufsize>=2) {
			strncpy(pvalue, buf, valuebufsize-1);
			return pvalue;
		}
		else {
			if (pvalue) { free(pvalue); }
			pvalue = malloc((sizeof(char)+1)*o);
			strncpy(pvalue, buf, o+1);
			return pvalue;
		}
	}
	return NULL;
}

void close_conf_file(void) {
	if (fd) {
		display_debug("Closing configuration file.", 0);
		fclose(fd);
		fd = NULL;
	}
}

int read_conf_int(const char *key, int defaultvalue){
	char *p = NULL;

	p = read_conf_string(key, p, 0); // 0 => fonction does the malloc
	if (p) {
		defaultvalue = atoi(p);
		free(p);
	}
	return defaultvalue;
}
