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

#define BUFFER_SIZE 81

//start of private prototypes

struct cstring_private_struct {
	size_t buffer_length;
};

void free_cstring_node(clist_node *node);

//end of private prototypes


cstring *new_cstring() {
	cstring *string;
	
	string = malloc(sizeof(cstring));
	string->private = malloc(sizeof(struct cstring_private_struct));
	string->length = 0;
	string->private->buffer_length = BUFFER_SIZE;
	string->string = malloc(sizeof(char) * BUFFER_SIZE);
	string->string[0] = '\0';
	
	return string;
}

void free_cstring(cstring *string) {
	free(string->private);
	free(string->string);
	free(string);
}

char *cstring_convert(cstring *self) {
	char *string;
	
	if (self == NULL) {
		return NULL;
	}
	
	// Note: this could be skipped.
	cstring_compact(self);
	
	free(self->private);
	string = (self->string);
	free(self);

	return string;
}

void *cstring_compact(cstring *self) {
	if (self != NULL) {
		self->private->buffer_length = self->length + 1;
		self->string = (char *)realloc(self->string, self->length + 1);
	}
}

void cstring_cut_at(cstring *self, size_t size) {
	if (self->length > size) {
		self->string[size] = '\0';
		self->length = size;
	}
}

clist *cstring_splitc(cstring *self, char delim, char quote) {
	clist *list;
	cstring *d, *q;

	d = new_cstring();
	q = new_cstring();

	cstring_addc(d, delim);
	cstring_addc(q, quote);

	list = cstring_split(self, d, q);

	free_cstring(d);
	free_cstring(q);

	return list;
}

clist *cstring_splits(cstring *self, const char delim[], const char quote[]) {
	clist *list;
	cstring *d, *q;

	d = new_cstring();
	q = new_cstring();

	cstring_adds(d, delim);
	cstring_adds(q, quote);

	list = cstring_split(self, d, q);

	free_cstring(d);
	free_cstring(q);

	return list;
}

clist *cstring_split(cstring *self, cstring *delim, cstring *quote) {
	clist *list;
	cstring *elem;
	clist_node *node;
	size_t i;
	int in_quote;
	
	list = new_clist();
	in_quote = 0;
	elem = NULL;
	for (i = 0 ; i < self->length ; i++) {
		if (quote->length > 0 && cstring_starts_with(self, quote, i)) {
			in_quote = !in_quote;
			i += quote->length -1;
		} else {
			if (elem == NULL) {
				elem = new_cstring();
				node = new_clist_node();
				node->data = elem;
				node->free_node = free_cstring_node;
				clist_add(list, node);
			}
			if (!in_quote && cstring_starts_with(self, delim, i)) {
				elem = new_cstring();
				node = new_clist_node();
				node->data = elem;
				node->free_node = free_cstring_node;
				clist_add(list, node);
				i += delim->length - 1;
			} else {
				cstring_addc(elem, self->string[i]);
			}
		}
	}
	
	return list;
}

cstring *cstring_substring(cstring *self, size_t start, size_t length) {
	cstring *sub;
	char *source;

	if (length == 0) {
		length = self->length - start;
	}

	sub = new_cstring();
	source = self->string;
	source = source + start;

	cstring_addns(sub, source, length);

	return sub;
}

int cstring_starts_with(cstring *self, cstring *find, size_t start_index) {
	return cstring_starts_withs(self, find->string, start_index);
}

int cstring_starts_withs(cstring *self, const char find[], size_t start_index) {
	size_t i;
	
	for (i = 0 ; self->string[start_index + i] == find[i] && self->string[start_index + i] != '\0' && find[i] != '\0' ; i++);

	return find[i] == '\0';
}

int cstring_ends_with(cstring *self, cstring *find, size_t start_index) {
	size_t i, indexi;

	indexi = self->length - find->length;
	if (indexi >= start_index) {
		indexi -=  start_index;
		for ( i = 0 ; self->length > (indexi + i) && find->length > i && self->string[indexi + i] == find->string[i] ; i++);
		return self->string[indexi + i] == find->string[i];
	}

	return 0;
}

int cstring_ends_withs(cstring *self, const char find[], size_t start_index) {
	cstring *cs;
	int result;

	cs = new_cstring();
	cstring_adds(cs, find);
	result = cstring_ends_with(self, cs, start_index);
	free_cstring(cs);

	return result;
}

long long cstring_find(cstring *self, cstring *find, size_t start_index) {
	return cstring_finds(self, find->string, start_index);
}

long long cstring_finds(cstring *self, const char find[], size_t start_index) {
	size_t i;

	for (i = start_index ; self->string[i] != '\0' ; i++) {
		if (cstring_starts_withs(self, find, i)) {
			return i;
		}
	}
	
	return -1;
}

int cstring_replace(cstring *self, cstring *from, cstring *to) {
	int occurs;
	size_t i, ii;
	cstring *tmp;

	occurs = 0;
	for (i = 0 ; self->string[i] != '\0' ; i++) {
		if (cstring_starts_with(self, from, i)) {
			occurs++;
			if (from->length == to->length) {
				for (ii = 0 ; ii < to->length ; ii++) {
					self->string[i + ii] = to->string[ii];
				}
			} else if (from->length > to->length) {
				for (ii = 0 ; ii < to->length ; ii++) {
					self->string[i + ii] = to->string[ii];
				}
				for ( ; ii <= ((self->length - i) - (from->length - to->length)) ; ii++) {
					self->string[i + ii] = self->string[i + ii + (from->length - to->length)];
				}
			} else {
				//TODO: this is not efficient...
				tmp = cstring_substring(self, i + from->length, 0);
				cstring_cut_at(self, i);
				cstring_add(self, to);
				cstring_add(self, tmp);
				free_cstring(tmp);
			}
			i += to->length -1;
		}
	}

	return occurs;
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

void cstring_addf(cstring *self, cstring *source, size_t indexi) {
	cstring_addfs(self, source->string, indexi);
}

void cstring_addn(cstring *self, cstring *source, size_t n) {
	cstring_addns(self, source->string, n);
}

void cstring_addfn(cstring *self, cstring *source, size_t indexi, size_t n) {
	cstring_addfns(self, source->string, indexi, n);
}

void cstring_addc(cstring *self, char source) {
	char source2[2];
	
	source2[0] = source;
	source2[1] = '\0';
	
	cstring_adds(self, source2);
}

void cstring_addx(cstring *self, int source) {
	cstring_addN(self, source, 16, 0);
}

void cstring_addX(cstring *self, int source) {
	cstring_addN(self, source, 16, 1);
}

void cstring_addi(cstring *self, int source) {
	cstring_addN(self, source, 10, 0);
}

void cstring_addN(cstring *self, int source, int radix, int cap) {
	int value;
	int tmp;
	cstring *string;
	
	string = new_cstring();
	value = source;
	
	if (value == 0) {
		cstring_addc(string, '0');
	} else {
		while (value > 0) {
			tmp = value % radix;
			if (tmp < 10) {
				cstring_addc(string, (char)('0' + tmp));
			} else {
				cstring_addc(string, (char)((cap ? 'A' : 'a') + (tmp - 10)));
			}
		
			value = value / radix;
		}
	}
	
	cstring_reverse(string);
	cstring_add(self, string);
	free_cstring(string);
}

void cstring_adds(cstring *self, const char source[]) {
	cstring_addfs(self, source, 0);
}

void cstring_addfs(cstring *self, const char source[], size_t indexi) {
	size_t ss, ptr;
	
	if (source != NULL) {
		ss = strlen(source) - indexi;
		while ((self->length + ss) > (self->private->buffer_length)) {
			self->private->buffer_length += BUFFER_SIZE;
			self->string = (char *)realloc(self->string, sizeof(char) * self->private->buffer_length);
		}
		
		for (ptr = self->length ; ptr <= (self->length + ss) ; ptr++) {
			self->string[ptr] = source[ptr - self->length + indexi];
		}
		self->length += ss;
	}
}

void cstring_addns(cstring *self, const char source[], size_t n) {
	cstring_addfns(self, source, 0, n);
}

void cstring_addfns(cstring *self, const char source[], size_t indexi, size_t n) {
	size_t i;
	char *tmp;
	
	for (i = indexi ; i<(n + indexi) && source[i] != '\0' ; i++);
	if (source[i] == '\0') {
		cstring_addfs(self, source, indexi);
	} else {
		tmp = (char *)malloc(sizeof(char) * (n + 1));
		strncpy(tmp, source + indexi, n);
		tmp[n] = '\0';
		cstring_adds(self, tmp);
		free(tmp);
	}
}

void free_cstring_node(clist_node *node) {
	if (node->data != NULL) {
		free_cstring((cstring *)node->data);
	}
	
	free(node);
}
