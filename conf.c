/*
  Name:         conf.c
  Author:       cLx
  Date:         21/06/11
  Description:  Simple solution for reading a configuration file
  Copyright:    CC-BY-NC 2011
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	unsigned char buf[200];
	char c;
	bool secondpass = false;
	
	if (!key || !key[0]) {
		fprintf(stderr, "read_conf_string : you have to specify the key's name !\n");
		return 0;
	}
			
	if (!fd) {
		fprintf(stderr, "Opening the configuration file...\n");
		
		fd = fopen(CONFIGURATION_FILE, "r");
		if (!fd) {
			perror("Unable to open "CONFIGURATION_FILE);
			return NULL;
		}
	}
	
	state = VRFY_KEY; i = 0; o = 0;
	while(state != END) {
		c = fgetc(fd);
		
		if (feof(fd)) {
			if (!secondpass) {
				if (state == IN_VALUE) {
					state = END;
				}
				else {
					fseek(fd, 0, SEEK_SET);
					secondpass = true;
					state = VRFY_KEY; i = 0;
				}
			}
			else {
				state = END;
				break;
			}
		}
		
		switch(state){
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
					state = WF_VALUE;
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
						fprintf(stderr, "Buffer full (o=%u) for reading configuration value of \"%s\" !\n", o, key);
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
			pvalue = malloc((sizeof(char)+1)*o);
			strncpy(pvalue, buf, o);
			return pvalue;
		}
	}
	return NULL;
}

void close_conf_file(void) {
	if (fd) {
		fprintf(stderr, "Closing configuration file.\n");
		fclose(fd);
		fd = NULL;
	}
}
