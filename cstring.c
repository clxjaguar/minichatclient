/*
  Name:        cstring.c
  Copyright:   niki (cc-by-nc) 2011
  Author:      niki
  Date:        2011-06-16
  Description: cstring is a collection of helper functions to manipulate string of text
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cstring.h"
#include "cstring_p.h"

void cstring_addn(cstring *self, int source, int radix, int cap);

int main (int argc, char **argv) {
	unsigned long i;
	cstring *string, *string2;
	
	// Check for memory leaks/check perfs
	printf("Running 100.000 operations...\n");
	for (i = 0 ; i < 100 * 1000 ; i++ ){
		string = new_cstring();
		cstring_reverse(string);
		cstring_adds(string, "Test.");
		cstring_clear(string);
		cstring_adds(string, "0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 Test.");
		cstring_reverse(string);
		cstring_addi(string, 98);
		free_cstring(string);
	}
	printf("Done.\n");
	
	string = new_cstring();
	
	string2 = new_cstring();
	cstring_adds(string2, "String 2");
	cstring_add(string, string2);
	free_cstring(string2);
	printf("You should see String 2: %s\n", string->string);
	
	cstring_clear(string);
	cstring_addi(string, 12);
	printf("You should see 12: %s\n", string->string);
	cstring_addx(string, 0xFF2);
	printf("You should see 12ff2: %s\n", string->string);
	cstring_clear(string);
	cstring_addX(string, 0xABAC);
	printf("You should see ABAC: %s\n", string->string);
	cstring_reverse(string);
	printf("You should see CABA: %s\n", string->string);
	free_cstring(string);
	
	return 0;
}

cstring *new_cstring() {
	cstring *string;
	
	string = malloc(sizeof(cstring));
	string->data = malloc(sizeof(struct cstring_private_struct));
	string->length = 0;
	string->data->buffer_length = BUFFER_SIZE;
	string->string = malloc(sizeof(char) * BUFFER_SIZE);
	string->string[0] = '\0';
	
	return string;
}

void free_cstring(cstring *string) {
	free(string->data);
	free(string->string);
	free(string);
}

void cstring_clear(cstring *self) {
	self->length = 0;
	self->string[0] = '\0';
}

void cstring_reverse(cstring *self) {
	size_t i;
	size_t last;
	char tmp;
	
	if (self->length > 0) {
		last = self->length - 1;
		for (i = 0 ; i <= (last / 2) ; i++) {
			tmp = self->string[i];
			self->string[i] = self->string[last - i];
			self->string[last - i] = tmp;
		}
	}
}

void cstring_add(cstring *self, cstring *source) {
	cstring_adds(self, source->string);
}

void cstring_addc(cstring *self, char source) {
	char source2[2];
	
	source2[0] = source;
	source2[1] = '\0';
	
	cstring_adds(self, source2);
}

void cstring_addx(cstring *self, int source) {
	cstring_addn(self, source, 16, 0);
}

void cstring_addX(cstring *self, int source) {
	cstring_addn(self, source, 16, 1);
}

void cstring_addi(cstring *self, int source) {
	cstring_addn(self, source, 10, 0);
}

void cstring_addn(cstring *self, int source, int radix, int cap) {
	int value;
	int tmp;
	cstring *string;
	
	string = new_cstring();
	value = source;
	while (value > 0) {
		tmp = value % radix;
		if (tmp < 10) {
			cstring_addc(string, '0' + tmp);
		} else {
			cstring_addc(string, (cap ? 'A' : 'a') + (tmp - 10));
		}
		
		value = value / radix;
	}
	
	cstring_reverse(string);
	cstring_add(self, string);
	free_cstring(string);
}

void cstring_adds(cstring *self, char *source) {
	size_t ss, ptr;
	
	ss = strlen(source);
	while ((self->length + ss) > (self->data->buffer_length)) {
		self->data->buffer_length += BUFFER_SIZE;
		self->string = (char *)realloc(self->string, sizeof(char) * self->data->buffer_length);
	}
	
	for (ptr = self->length ; ptr <= (self->length + ss) ; ptr++) {
		self->string[ptr] = source[ptr - self->length];
	}
	self->length += ss;
}
