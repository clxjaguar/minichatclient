#ifndef CSTRING_H
#define CSTRING_H

#include "cstring_p.h"

typedef struct cstring_struct cstring;
struct cstring_struct {
	char *string;
	size_t length;
	cstring_private *private;
};

/**
 * Create (and allocate the memory for) a new String.
 * Do not forget to call free_cstring(cstring) when done.
 */
cstring *new_cstring();

/**
 * Free all the resources allocated for this cstring.
 */
void free_cstring(cstring *string);

/**
 * Add another cstring at the end of the first one.
 */
void cstring_add(cstring *self, cstring *source);

/**
 * Add a another cstring at the end of the given cstring, starting from index.
 */
void cstring_addf(cstring *self, cstring *source, int index);

/**
 * Add another string at the end of the given cstring, up to N chars long.
 */
void cstring_addn(cstring *self, cstring *source, int n);

/**
 * Add another cstring at the end of the given cstring, starting from index, up to N chars long.
 */
void cstring_addfn(cstring *self, cstring *source, int index, int n);

/**
 * Add a char at the end of the given cstring.
 */
void cstring_addc(cstring *self, char source);

/**
 * Add a string (a sequence of char that MUST end with '\0') at the end of the given cstring.
 */
void cstring_adds(cstring *self, char *source);

/**
 * Add a string (a sequence of char that MUST end with '\0') at the end of the given cstring, starting from index.
 */
void cstring_addfs(cstring *self, char *source, int index);

/**
 * Add a string (a sequence of char that MAY end with '\0') at the end of the given cstring, up to N chars long.
 */
void cstring_addns(cstring *self, char *source, int n);

/**
 * Add a string (a sequence of char that MAY end with '\0') at the end of the given cstring, starting from index,
 * up to N chars long.
 */
void cstring_addfns(cstring *self, char *source, int index, int n);

/**
 * Add an int at the end of the given cstring.
 */
void cstring_addi(cstring *self, int source);

/**
 * Add an int at the end of the given cstring, in hexadecimal format (lower case).
 */
void cstring_addx(cstring *self, int source);

/**
 * Add an int at the end of the given cstring, in hexadecimal format (upper case).
 */
void cstring_addX(cstring *self, int source);

/**
 * Add a number to the given cstring, in the given format.
 * self: the cstring
 * source: is the number to add
 * radix: 8, 10 or 16 (the radix of the number to add -- anything above 10 will be assigned a letter from a to z (then, kaboom))
 * cap: capitalize (upper case) the letters if any, 0 = no, 1 = yes
 */
void cstring_addN(cstring *self, int source, int radix, int cap);

/**
 * Cut the cstring at the given size if it is greater.
 * E.g.: it will have (at most) this many characters (without counting NUL) in it after.
 */
void cstring_cut_at(cstring *self, size_t size);

clist *cstring_split(cstring *self, cstring *delim, cstring *quote);
clist *cstring_splits(cstring *self, char *delim, char *quote);
clist *cstring_splitc(cstring *self, char delim, char quote);

/**
 * Reverse the given cstring.
 */
void cstring_reverse(cstring *self);

/*
 * Clear (truncate to 0) the given cstring.
 */
void cstring_clear(cstring *self);

/**
 * Convert this cstring into a string (this means that you MUST NOT call free_cstring nor use the cstring anymore).
 * NULL will return NULL.
 */
char *cstring_convert(cstring *self);

#endif
