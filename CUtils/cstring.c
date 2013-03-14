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
#include <math.h>

/* Windows (and maybe others?) doesn't know about strnlen */
#ifndef strnlen
	#define strnlen(a, b) strlen(a)
#endif

#include "cstring.h"
#include "net.h"

#define BUFFER_SIZE 81

#ifdef WIN32
		#define CSTRING_SEP '\\'
#else
		#define CSTRING_SEP '/'
#endif

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

/**
 * implement cstring_readline and cstring_readnet.
 */
int cstring_readlinenet(cstring *self, FILE *file, int fd);

//end of private prototypes


cstring *cstring_new() {
	cstring *string;

	string = malloc(sizeof(cstring));
	string->priv = malloc(sizeof(struct cstring_private_struct));
	string->length = 0;
	string->priv->buffer_length = BUFFER_SIZE;
	string->string = malloc(sizeof(char) * BUFFER_SIZE);
	string->string[0] = '\0';

	return string;
}

void cstring_free(cstring *string) {
	if (!string)
		return;
	
	free(string->priv);
	free(string->string);
	
	free(string);
}

char *cstring_convert(cstring *self) {
	char *string;

	if (!self)
		return NULL;
	
	// Note: this could be skipped.
	cstring_compact(self);

	free(self->priv);
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
		self->priv->buffer_length = self->length + 1;
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

	if (delim)
		cstring_addc(d, delim);
	if (quote)
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

	if (delim)
		cstring_adds(d, delim);
	if (quote)
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
	int hasdelim;

	hasdelim = delim && delim->length > 0;

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
			if (!in_quote && hasdelim && cstring_starts_with(self, delim, i)) {
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
			return (long long)i;
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
				return (long long)i;
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

//TODO: improve this or don't use it
int cstring_pow(int a, int b);

int cstring_pow(int val, int power) {
	int rep = 1;
	while (power > 0) {
		rep *= val;
		power--;
	}
	return rep;
}

void cstring_addd(cstring *self, double source, int afterDot) {
	int big, low;
	int i, add0;
	cstring *slow;

	big = (int)source;
	low = 0;
	if (afterDot > 0)
		low = (int)((source - big) * cstring_pow(10, afterDot));

	slow = cstring_new();
	if (afterDot > 0) {
		cstring_addi(slow, low);
		add0 = afterDot - slow->length;
		cstring_clear(slow);
		cstring_addc(slow, '.');
		for (i = 0 ; i < add0 ; i++)
			cstring_addi(slow, 0);
		cstring_addi(slow, low);
	}

	cstring_addi(self, big);
	cstring_add(self, slow);
}

void cstring_addN(cstring *self, int source, int radix, int cap) {
	int value;
	int tmp;
	cstring *string;

	string = cstring_new();
	value = source > 0 ? source : -source;

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
	if (source < 0)
		cstring_addc(self, '-');
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
		while ((self->length + ss) >= (self->priv->buffer_length)) {
			self->priv->buffer_length += BUFFER_SIZE;
			self->string = (char *) realloc(self->string, sizeof(char)
					* self->priv->buffer_length);
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
	return cstring_readlinenet(self, file, -1);
}

int cstring_readnet(cstring *self, int fd) {
	return cstring_readlinenet(self, NULL, fd);
}

int cstring_readit(char *buffer, FILE *file, int fd, int max, size_t *size) {
	int net_ok;

	if (file) {
		fgets(buffer, max, file);
		*size = strlen(buffer);
	}
	else {
		net_ok = net_read(fd, buffer, max) >= 0;
		if (!net_ok) {
			buffer[0] = '\0';
			*size = 0;
			return 0;
		} else {
			*size = strnlen(buffer, max);
		}
	}

	return 1;
}

int cstring_readlinenet(cstring *self, FILE *file, int fd) {
	char buffer[BUFFER_SIZE];
	size_t size;
	int full_line;

	// sanity check:
	if (!file && fd < 0)
		return 0;

	buffer[BUFFER_SIZE - 1] = '\0'; // just in case

	if (fd >= 0 || !feof(file)) {
		cstring_clear(self);
		buffer[0] = '\0';
		
		// Note: strlen() could return 0 if the file contains \0
		// at the start of a line
		if (!cstring_readit(buffer, file, fd, 80, &size))
			return 0;
		
		full_line = ((file && feof(file)) || size == 0 || buffer[size - 1] == '\n');
		cstring_remove_crlf(buffer, size);
		cstring_adds(self, buffer);
		
		// No luck, we need to continue getting data
		while (!full_line) {
			if (!cstring_readit(buffer, file, fd, 80, &size))
				break;

			full_line = ((file && feof(file)) || size == 0 || buffer[size - 1] == '\n');
			cstring_remove_crlf(buffer, size);
			cstring_adds(self, buffer);
		}
		return 1;
	}
	return 0;
}

cstring *cstring_combine(cstring *self, cstring *file) {
	cstring *str;

	str = cstring_clone(self);
	cstring_addc(str, CSTRING_SEP);
	cstring_add(str, file);

	return str;
}

cstring *cstring_combines(cstring *self, const char file[]) {
	cstring *csfile;
	cstring *rep;

	csfile = cstring_clones(file);
	rep = cstring_combine(self, csfile);
	cstring_free(csfile);
	return rep;
}

cstring *cstring_scombines(const char dir[], const char file[]) {
	cstring *csfile, *csdir;
	cstring *rep;

	csfile = cstring_clones(file);
	csdir = cstring_clones(dir);
	rep = cstring_combine(csdir, csfile);
	cstring_free(csfile);
	cstring_free(csdir);
	return rep;
}

cstring *cstring_getdir(cstring *path) {
	cstring *result;
	ssize_t i;

	i = path->length - 1;
	if (i >= 0 && path->string[i] == CSTRING_SEP)
		i--;
	for ( ; i >= 0 && path->string[i] != CSTRING_SEP ; i--);

	if (i < 0)
		return cstring_new();

	result = cstring_clone(path);
	cstring_cut_at(result, i);
	return result;
}

cstring *cstring_getdirs(const char path[]) {
	cstring *copy, *result;

	copy = cstring_clones(path);
	result = cstring_getdir(copy);
	cstring_free(copy);
	return result;
}

cstring *cstring_getfile(cstring *path) {
	cstring *result;
	ssize_t i;

	i = path->length - 1;
	if (i >= 0 && path->string[i] == CSTRING_SEP)
		i--;
	for ( ; i >= 0 && path->string[i] != CSTRING_SEP ; i--);

	if (i < 0 || (size_t)(i + 1) >= path->length)
		return cstring_new();

	result = cstring_clones(path->string + i + 1);
	return result;
}

cstring *cstring_getfiles(const char path[]) {
	cstring *copy, *result;

	copy = cstring_clones(path);
	result = cstring_getfile(copy);
	cstring_free(copy);
	return result;
}
