/* C string replacement routine - sorely needed in libc
 * v1 written Nov 26th, 2004
 * initial version by Christian Lavoie
 * some improvements by Kacper Wysocki
 * This file is in the public domain. Yours to do with as you please.
 * Modified by cLx (06/09/11)
 * */
#include <stdlib.h>
#include <string.h>

void strrep(char* in, char** out, char* old, char* new) {
	char* temp;
	char* found = strstr(in, old);
	if(!found) {
		*out = malloc(strlen(in) + 1);
		strcpy(*out, in);
		return;
	}

	int idx = found - in;
	if (!*out) { *out = malloc(strlen(in) - strlen(old) + strlen(new) + 1); }
	else { *out = realloc(*out, strlen(in) - strlen(old) + strlen(new) + 1); }
	
	strncpy(*out, in, idx);
	strcpy(*out + idx, new);
	strcpy(*out + idx + strlen(new), in + idx + strlen(old));

	
	temp = malloc(idx+strlen(new)+1);
	strncpy(temp,*out,idx+strlen(new));
	temp[idx + strlen(new)] = '\0';
	
	strrep(found + strlen(old), out, old, new);
	temp = realloc(temp, strlen(temp) + strlen(*out) + 1);
	strcat(temp,*out);
	free(*out);
	*out = temp; 
}

/*
void test(int argc, char* argv[]) {
	//char* in = "that's my string for testing ''s in 's\n";
	char* out = malloc(1);
	char* in = malloc(400);
	while(fgets(in,400,stdin) != NULL){
		strrep(in, &out, "'", "\\'");
		printf("%s", out);
	}
}
*/
