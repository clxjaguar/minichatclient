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

// Old
cstring *new_cstring() {
	return cstring_new();
}
void free_cstring(cstring *string) {
	cstring_free(string);
}
//

//start of private prototypes

struct cstring_private_struct {
	size_t buffer_length;
};

/**
 * convert CRLF to LF, do not store the final \\n.
 * @param data
 * @param size
 */
void cstring_remove_crlf(char data[], size_t size);

//end of private prototypes


cstring *cstring_new() {
	cstring *string;

	string = malloc(sizeof(cstring));
	string->private = malloc(sizeof(struct cstring_private_struct));
	string->length = 0;
	string->private->buffer_length = BUFFER_SIZE;
	string->string = malloc(sizeof(char) * BUFFER_SIZE);
	string->string[0] = '\0';

	return string;
}

void cstring_free(cstring *string) {
	free(string->private);
	free(string->string);
	free(string);
}

char *cstring_convert(cstring *self) {
	char *string;

	if (self == NULL)
		return NULL;
	
	// Note: this could be skipped.
	cstring_compact(self);

	free(self->private);
	string = (self->string);
	free(self);

	return string;
}

cstring *cstring_clone(cstring *self) {
	if (self == NULL)
		return NULL;
	
	return cstring_clones(self->string);
}

char *cstring_sclone(cstring *self) {
	if (self == NULL)
		return NULL;
	
	return cstring_convert(cstring_clone(self));
}

char *cstring_sclones(const char self[]) {
	return cstring_convert(cstring_clones(self));
}

cstring *cstring_clones(const char self[]) {
	cstring *clone;
	
	if (self == NULL)
		return NULL;
	
	clone = cstring_new();
	cstring_adds(clone, self);
	
	return clone;
}

void cstring_compact(cstring *self) {
	if (self != NULL) {
		self->private->buffer_length = self->length + 1;
		self->string = (char *) realloc(self->string, self->length + 1);
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

	d = cstring_new();
	q = cstring_new();

	cstring_addc(d, delim);
	cstring_addc(q, quote);

	list = cstring_split(self, d, q);

	cstring_free(d);
	cstring_free(q);

	return list;
}

clist *cstring_splits(cstring *self, const char delim[], const char quote[]) {
	clist *list;
	cstring *d, *q;

	d = cstring_new();
	q = cstring_new();

	cstring_adds(d, delim);
	cstring_adds(q, quote);

	list = cstring_split(self, d, q);

	cstring_free(d);
	cstring_free(q);

	return list;
}

clist *cstring_split(cstring *self, cstring *delim, cstring *quote) {
	clist *list;
	cstring *elem;
	clist_node *node;
	size_t i;
	int in_quote;

	list = clist_new();
	in_quote = 0;
	elem = NULL;
	for (i = 0; i < self->length; i++) {
		if (quote->length > 0 && cstring_starts_with(self, quote, i)) {
			in_quote = !in_quote;
			i += quote->length - 1;
		} else {
			if (elem == NULL) {
				elem = cstring_new();
				node = clist_node_new();
				node->data = elem;
				node->free_data = cstring_free;
				clist_add(list, node);
			}
			if (!in_quote && cstring_starts_with(self, delim, i)) {
				elem = cstring_new();
				node = clist_node_new();
				node->data = elem;
				node->free_data = cstring_free;
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

	sub = cstring_new();
	source = self->string;
	source = source + start;

	cstring_addns(sub, source, length);

	return sub;
}

int cstring_starts_with(cstring *self, cstring *find, size_t start_index) {
	return cstring_sstarts_withs(self->string, find->string, start_index);
}

int cstring_starts_withs(cstring *self, const char find[], size_t start_index) {
	return cstring_sstarts_withs(self->string, find, start_index);
}

int cstring_sstarts_withs(const char string[], const char find[],
		size_t start_index) {
	size_t i;

	for (i = 0; string[start_index + i] == find[i] && string[start_index + i]
			!= '\0' && find[i] != '\0'; i++)
		;

	return find[i] == '\0';
}

int cstring_ends_with(cstring *self, cstring *find, size_t start_index) {
	size_t i, indexi;

	indexi = self->length - find->length;
	if (indexi >= start_index) {
		indexi -= start_index;
		for (i = 0; self->length > (indexi + i) && find->length > i
				&& self->string[indexi + i] == find->string[i]; i++)
			;
		return self->string[indexi + i] == find->string[i];
	}

	return 0;
}

int cstring_ends_withs(cstring *self, const char find[], size_t start_index) {
	cstring *cs;
	int result;

	cs = cstring_new();
	cstring_adds(cs, find);
	result = cstring_ends_with(self, cs, start_index);
	cstring_free(cs);

	return result;
}

long long cstring_find(cstring *self, cstring *find, size_t start_index) {
	return cstring_finds(self, find->string, start_index);
}

long long cstring_finds(cstring *self, const char find[], size_t start_index) {
	size_t i;

	if (start_index >= self->length || find == NULL) {
		return -1;
	}

	for (i = start_index; self->string[i] != '\0'; i++) {
		if (cstring_starts_withs(self, find, i)) {
			return i;
		}
	}

	return -1;
}

long long cstring_find_anys(cstring *self, const char find[],
		size_t start_index) {
	size_t i, ii;

	if (start_index >= self->length || find == NULL) {
		return -1;
	}

	for (i = start_index; self->string[i] != '\0'; i++) {
		for (ii = 0; find[ii] != '\0'; ii++) {
			if (self->string[i] == find[ii]) {
				return i;
			}
		}
	}

	return -1;
}

int cstring_replaces(cstring *self, const char from[], const char to[]) {
	cstring *cfrom, *cto;
	int rep;
	
	cfrom = cstring_clones(from);
	cto = cstring_clones(to);
	
	rep = cstring_replace(self, cfrom, cto);
	
	cstring_free(cfrom);
	cstring_free(cto);
	
	return rep;
}

int cstring_replace(cstring *self, cstring *from, cstring *to) {
	int occurs;
	size_t i, ii;
	cstring *tmp;

	occurs = 0;
	for (i = 0; self->string[i] != '\0'; i++) {
		if (cstring_starts_with(self, from, i)) {
			occurs++;
			if (from->length == to->length) {
				for (ii = 0; ii < to->length; ii++) {
					self->string[i + ii] = to->string[ii];
				}
			} else if (from->length > to->length) {
				for (ii = 0; ii < to->length; ii++) {
					self->string[i + ii] = to->string[ii];
				}
				for (; ii <= ((self->length - i) - (from->length - to->length)); ii++) {
					self->string[i + ii] = self->string[i + ii + (from->length
							- to->length)];
				}
			} else {
				//TODO: this is not efficient...
				tmp = cstring_substring(self, i + from->length, 0);
				cstring_cut_at(self, i);
				cstring_add(self, to);
				cstring_add(self, tmp);
				cstring_free(tmp);
			}
			i += to->length - 1;
		}
	}

	if (occurs > 0)
		self->length = strlen(self->string); //TODO: NOT EFFICIENT!!!

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
		for (i = 0; i <= (last / 2); i++) {
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

	string = cstring_new();
	value = source;

	if (value == 0) {
		cstring_addc(string, '0');
	} else {
		while (value > 0) {
			tmp = value % radix;
			if (tmp < 10) {
				cstring_addc(string, (char) ('0' + tmp));
			} else {
				cstring_addc(string, (char) ((cap ? 'A' : 'a') + (tmp - 10)));
			}

			value = value / radix;
		}
	}

	cstring_reverse(string);
	cstring_add(self, string);
	cstring_free(string);
}

void cstring_adds(cstring *self, const char source[]) {
	cstring_addfs(self, source, 0);
}

void cstring_addfs(cstring *self, const char source[], size_t indexi) {
	size_t ss, ptr;

	if (source != NULL && strlen(source) > indexi) {
		ss = strlen(source) - indexi;
		while ((self->length + ss) >= (self->private->buffer_length)) {
			self->private->buffer_length += BUFFER_SIZE;
			self->string = (char *) realloc(self->string, sizeof(char)
					* self->private->buffer_length);
		}

		for (ptr = self->length; ptr <= (self->length + ss); ptr++) {
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

	for (i = indexi; i < (n + indexi) && source[i] != '\0'; i++)
		;
	if (source[i] == '\0') {
		cstring_addfs(self, source, indexi);
	} else {
		tmp = (char *) malloc(sizeof(char) * (n + 1));
		strncpy(tmp, source + indexi, n);
		tmp[n] = '\0';
		cstring_adds(self, tmp);
		free(tmp);
	}
}

cstring *cstring_trimc(cstring *self, char car, int start, int end) {
		size_t n, size;
		size_t i;
		
		n = 0;
		size = self->length;
		
		if (start) {
			for (n = 0 ; n <= self->length && self->string[n] == car ; n++);
			size -= n;
		}

		if (end) {
			for (i = self->length - 1 ; i < self->length && self->string[i] == car ; i--)
				size --;
		}
		
		return cstring_substring(self, n, size);
}

void cstring_remove_crlf(char data[], size_t size) {
	if (size > 0 && data[size - 1] == '\n') {
		data[size - 1] = '\0';
		if (size > 1 && data[size - 2] == '\r')
			data[size - 2] = '\0';
	}
}

int cstring_readline(cstring *self, FILE *file) {
	char buffer[BUFFER_SIZE];
	size_t size;
	int full_line;

	buffer[BUFFER_SIZE - 1] = '\0'; // just in case

	if (file != NULL) {
		if (!feof(file)) {
			cstring_clear(self);
			buffer[0] = '\0';
			
			fgets(buffer, 80, file);
			// Note: strlen() could return 0 if the file contains \0
			// at the start of a line
			size = strlen(buffer);
			full_line = (feof(file) || size == 0 || buffer[size - 1] == '\n');
			cstring_remove_crlf(buffer, size);
			cstring_adds(self, buffer);
			
			// No luck, we need to continue getting data
			while (!full_line) {
				fgets(buffer, 80, file);
				size = strlen(buffer);
				full_line = (feof(file) || size == 0 || buffer[size - 1] == '\n');
				cstring_remove_crlf(buffer, size);
				cstring_adds(self, buffer);
			}

			return 1;
		}
	}

	return 0;
}