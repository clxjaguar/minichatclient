#ifndef CSTRING_H
#define CSTRING_H

#include "cstring_p.h"

typedef struct cstring_struct cstring;
struct cstring_struct {
	struct cstring_private_struct *data;
	size_t length;
	char *string;
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
 * Add a char at the end of the given cstring.
 */
void cstring_addc(cstring *self, char source);

/**
 * Add a string (a sequence of char that MUST end with '\0') at the end of the given cstring.
 */
void cstring_adds(cstring *self, char *source);

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
void cstring_addn(cstring *self, int source, int radix, int cap);

/**
 * Reverse the given cstring.
 */
void cstring_reverse(cstring *self);

/*
 * Clear (truncate to 0) the given cstring.
 */
void cstring_clear(cstring *self);

#endif
